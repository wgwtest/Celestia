// realmodelbackend.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "realmodelbackend.h"

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

#include <celengine/adapter/sceneviewmodel.h>
#include <celengine/adapter/starrenderassets.h>
#include <celengine/controller/observer.h>
#include <celengine/controller/selection.h>
#include <celengine/controller/simulation.h>
#include <celengine/model/dsodb.h>
#include <celengine/model/stardb.h>
#include <celengine/model/universe.h>
#include <celengine/model/urlmanager.h>
#include <celengine/view3d/meshmanager.h>
#include <celengine/view3d/texmanager.h>
#include <celestia/configfile.h>
#include <celestia/loaddso.h>
#include <celestia/loadsso.h>
#include <celestia/loadstars.h>
#include <celutil/logger.h>

namespace celestia::runtime::model
{
namespace
{

constexpr double DefaultJulianDay{ 2451545.0 };

class ScopedCurrentPath
{
public:
    explicit ScopedCurrentPath(const std::filesystem::path& path)
    {
        std::error_code ec;
        previous_ = std::filesystem::current_path(ec);
        if (ec)
            return;

        std::filesystem::current_path(path, ec);
        active_ = !ec;
    }

    ~ScopedCurrentPath()
    {
        if (!active_)
            return;

        std::error_code ec;
        std::filesystem::current_path(previous_, ec);
    }

    bool active() const noexcept { return active_; }

private:
    std::filesystem::path previous_;
    bool active_{ false };
};

Selection
selectRepresentativeObject(Simulation& simulation)
{
    if (auto earth = simulation.findObjectFromPath("Sol/Earth"); !earth.empty())
        return earth;

    if (auto earth = simulation.findObjectFromPath("Earth"); !earth.empty())
        return earth;

    auto* universe = simulation.getUniverse();
    if (universe == nullptr || universe->getStarCatalog() == nullptr ||
        universe->getStarCatalog()->size() == 0)
    {
        return {};
    }

    return Selection(universe->getStarCatalog()->getStar(0));
}

std::string
frameSummary(const std::filesystem::path& dataRoot, std::uint64_t frameId)
{
    std::ostringstream output;
    output << "real Celestia model frame " << frameId
           << " dataRoot=" << dataRoot.generic_string();
    return output.str();
}

std::string
resourcePath(const std::filesystem::path& path)
{
    return path.generic_string();
}

void
appendCatalogResource(ViewFrame& frame,
                      std::string id,
                      const std::filesystem::path& relativePath,
                      bool required)
{
    if (relativePath.empty())
        return;

    for (const auto& resource : frame.resources)
    {
        if (resource.id == id)
            return;
    }

    ViewFrameResource resource;
    resource.id = std::move(id);
    resource.kind = "catalog";
    resource.package = "celestia-core";
    resource.relativePath = resourcePath(relativePath);
    resource.required = required;
    frame.resources.push_back(std::move(resource));
}

void
appendConfiguredResources(ViewFrame& frame, const CelestiaConfig& config)
{
    appendCatalogResource(frame, "res:catalog:stars", config.paths.starDatabaseFile, true);
    appendCatalogResource(frame, "res:catalog:starnames", config.paths.starNamesFile, false);

    for (std::size_t i = 0; i < config.paths.solarSystemFiles.size(); ++i)
    {
        appendCatalogResource(frame,
                              i == 0 ? "res:catalog:solarsys" : "res:catalog:solarsys:" + std::to_string(i),
                              config.paths.solarSystemFiles[i],
                              i == 0);
    }

    for (std::size_t i = 0; i < config.paths.starCatalogFiles.size(); ++i)
    {
        appendCatalogResource(frame,
                              "res:catalog:star-extension:" + std::to_string(i),
                              config.paths.starCatalogFiles[i],
                              false);
    }

    for (std::size_t i = 0; i < config.paths.dsoCatalogFiles.size(); ++i)
    {
        appendCatalogResource(frame,
                              "res:catalog:dso:" + std::to_string(i),
                              config.paths.dsoCatalogFiles[i],
                              false);
    }
}

void
ensureCelestiaLogger()
{
    if (celestia::util::GetLogger() == nullptr)
        celestia::util::CreateLogger(celestia::util::Level::Warning);
}

class RealModelBackend final : public SimulationBackend
{
public:
    bool load(const RuntimeDataPaths& paths) override
    {
        const std::filesystem::path root{ paths.dataRoot };
        if (root.empty() ||
            !std::filesystem::exists(root / "celestia.cfg") ||
            !std::filesystem::is_directory(root / "data"))
        {
            return false;
        }

        ensureCelestiaLogger();
        ScopedCurrentPath currentPath(root);
        if (!currentPath.active())
            return false;

        auto texturePaths = std::make_shared<engine::TexturePaths>();
        auto geometryPaths = std::make_shared<engine::GeometryPaths>();
        auto config = std::make_unique<CelestiaConfig>();
        if (!ReadCelestiaConfig("celestia.cfg", *config, *texturePaths))
            return false;

        auto universe = std::make_unique<Universe>(std::make_unique<engine::UrlManager>());
        StarRenderAssets::setStarTextures(config->starTextures);

        auto starCatalog = loadStars(*config, nullptr, *geometryPaths, *texturePaths, *universe->getUrlManager());
        if (starCatalog == nullptr)
            return false;
        universe->setStarCatalog(std::move(starCatalog));

        auto dsoCatalog = loadDSO(*config, nullptr, *geometryPaths, *universe->getUrlManager());
        if (dsoCatalog == nullptr)
            return false;
        universe->setDSOCatalog(std::move(dsoCatalog));

        loadSSO(*config, nullptr, *universe, *geometryPaths, *texturePaths, *universe->getUrlManager());

        auto observerSettings = std::make_shared<engine::ObserverSettings>();
        if (config->observer.alignCameraToSurfaceOnLand)
            observerSettings->flags = engine::ObserverFlags::AlignCameraToSurfaceOnLand;

        auto simulation = std::make_unique<Simulation>(std::move(universe), observerSettings);
        simulation->setTime(DefaultJulianDay);
        simulation->setSelection(selectRepresentativeObject(*simulation));

        dataRoot_ = root;
        config_ = std::move(config);
        texturePaths_ = std::move(texturePaths);
        geometryPaths_ = std::move(geometryPaths);
        simulation_ = std::move(simulation);
        frameId_ = 0;
        return true;
    }

    void setTime(double time) override
    {
        if (simulation_ != nullptr)
            simulation_->setTime(time);
    }

    void step(double dt) override
    {
        if (simulation_ != nullptr)
            simulation_->update(dt);
        ++frameId_;
    }

    ViewFrame snapshot() const override
    {
        if (simulation_ == nullptr)
            return {};

        auto frame = SceneViewModel::buildSelectionSnapshot(*simulation_);
        frame.frameId = frameId_;
        frame.summary = frameSummary(dataRoot_, frameId_);
        if (config_ != nullptr)
            appendConfiguredResources(frame, *config_);
        return frame;
    }

private:
    std::filesystem::path dataRoot_;
    std::unique_ptr<CelestiaConfig> config_;
    std::shared_ptr<engine::TexturePaths> texturePaths_;
    std::shared_ptr<engine::GeometryPaths> geometryPaths_;
    std::unique_ptr<Simulation> simulation_;
    std::uint64_t frameId_{ 0 };
};

} // end unnamed namespace

std::unique_ptr<SimulationBackend>
createRealModelBackend()
{
    return std::make_unique<RealModelBackend>();
}

} // namespace celestia::runtime::model
