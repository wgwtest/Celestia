// view3dloop.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "view3dloop.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <SDL.h>
#include <epoxy/gl.h>

namespace celestia::runtime::view3d
{
namespace
{

constexpr double Pi = 3.14159265358979323846;

void
setError(std::string* errorMessage, std::string message)
{
    if (errorMessage != nullptr)
        *errorMessage = std::move(message);
}

std::string
sdlError(std::string prefix)
{
    std::string message = std::move(prefix);
    message += ": ";
    message += SDL_GetError();
    return message;
}

int
clampInt(int value, int low, int high)
{
    return std::max(low, std::min(value, high));
}

} // end unnamed namespace

struct View3DLoop::Impl
{
    bool start(const View3DLoopOptions& requestedOptions, std::string* errorMessage)
    {
        if (window != nullptr)
            return true;

        options = requestedOptions;
        options.width = std::max(320, options.width);
        options.height = std::max(240, options.height);

        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            setError(errorMessage, sdlError("failed to initialize SDL video"));
            return false;
        }
        sdlInitialized = true;

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

        const auto flags = SDL_WINDOW_OPENGL | (options.visible ? SDL_WINDOW_SHOWN : SDL_WINDOW_HIDDEN);
        window = SDL_CreateWindow(options.title.c_str(),
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  options.width,
                                  options.height,
                                  flags);
        if (window == nullptr)
        {
            setError(errorMessage, sdlError("failed to create View3D SDL window"));
            stop();
            return false;
        }

        context = SDL_GL_CreateContext(window);
        if (context == nullptr)
        {
            setError(errorMessage, sdlError("failed to create View3D OpenGL context"));
            stop();
            return false;
        }

        if (SDL_GL_MakeCurrent(window, context) != 0)
        {
            setError(errorMessage, sdlError("failed to activate View3D OpenGL context"));
            stop();
            return false;
        }

        SDL_GL_SetSwapInterval(0);
        glViewport(0, 0, options.width, options.height);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        const auto* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
        const auto* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        std::ostringstream info;
        info << (vendor != nullptr ? vendor : "unknown-vendor")
             << " / "
             << (renderer != nullptr ? renderer : "unknown-renderer");
        rendererInfo = info.str();

        clearBackground();
        SDL_GL_SwapWindow(window);
        return true;
    }

    bool render(const protocol::SceneFrame& frame, std::string* errorMessage)
    {
        if (window == nullptr || context == nullptr)
        {
            setError(errorMessage, "View3D loop is not running");
            return false;
        }

        SDL_Event event;
        while (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT)
            {
                closeRequested = true;
                pendingInput.push_back(inputEvent("window", "Quit"));
            }
            else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
            {
                auto input = inputEvent("keyboard", event.type == SDL_KEYDOWN ? "KeyDown" : "KeyUp");
                input.key = SDL_GetKeyName(event.key.keysym.sym);
                input.modifiers = modifiers(static_cast<SDL_Keymod>(event.key.keysym.mod));
                pendingInput.push_back(std::move(input));
            }
            else if (event.type == SDL_MOUSEMOTION)
            {
                auto input = inputEvent("mouse", "MouseMove");
                input.pointer = { static_cast<double>(event.motion.x), static_cast<double>(event.motion.y) };
                pendingInput.push_back(std::move(input));
            }
            else if (event.type == SDL_MOUSEWHEEL)
            {
                auto input = inputEvent("mouse", "MouseWheel");
                input.wheel = { static_cast<double>(event.wheel.x), static_cast<double>(event.wheel.y) };
                pendingInput.push_back(std::move(input));
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
            {
                auto input = inputEvent("mouse", event.type == SDL_MOUSEBUTTONDOWN ? "MouseButtonDown" : "MouseButtonUp");
                input.key = std::to_string(event.button.button);
                input.pointer = { static_cast<double>(event.button.x), static_cast<double>(event.button.y) };
                pendingInput.push_back(std::move(input));
            }
        }

        if (SDL_GL_MakeCurrent(window, context) != 0)
        {
            setError(errorMessage, sdlError("failed to activate View3D OpenGL context"));
            return false;
        }

        glViewport(0, 0, options.width, options.height);
        clearBackground();
        glEnable(GL_SCISSOR_TEST);
        drawStars(frame);
        drawOrbit(frame);
        drawBody(frame);
        glDisable(GL_SCISSOR_TEST);
        SDL_GL_SwapWindow(window);

        ++renderedFrames;
        return !closeRequested;
    }

    std::vector<protocol::ViewInputEvent> drainInputEvents()
    {
        auto events = std::move(pendingInput);
        pendingInput.clear();
        return events;
    }

    void stop()
    {
        if (context != nullptr)
        {
            SDL_GL_DeleteContext(context);
            context = nullptr;
        }

        if (window != nullptr)
        {
            SDL_DestroyWindow(window);
            window = nullptr;
        }

        if (sdlInitialized)
        {
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            SDL_Quit();
            sdlInitialized = false;
        }
    }

    bool isRunning() const
    {
        return window != nullptr && context != nullptr;
    }

    void clearBackground()
    {
        glDisable(GL_SCISSOR_TEST);
        glClearColor(0.015f, 0.025f, 0.055f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void clearRect(int x, int y, int width, int height, float red, float green, float blue)
    {
        const auto clampedX = clampInt(x, 0, options.width - 1);
        const auto clampedY = clampInt(y, 0, options.height - 1);
        const auto clampedWidth = clampInt(width, 1, options.width - clampedX);
        const auto clampedHeight = clampInt(height, 1, options.height - clampedY);

        glScissor(clampedX, clampedY, clampedWidth, clampedHeight);
        glClearColor(red, green, blue, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void drawStars(const protocol::SceneFrame& frame)
    {
        const auto starCount = std::max<std::size_t>(frame.stars.size(), 18);
        for (std::size_t i = 0; i < starCount; ++i)
        {
            const auto x = 24 + static_cast<int>((i * 73) % static_cast<std::size_t>(std::max(80, options.width - 48)));
            const auto y = 24 + static_cast<int>((i * 41) % static_cast<std::size_t>(std::max(80, options.height - 48)));
            const auto brightness = 0.55f + static_cast<float>((i % 5) * 0.09f);
            clearRect(x, y, 2, 2, brightness, brightness, std::min(1.0f, brightness + 0.1f));
        }
    }

    void drawOrbit(const protocol::SceneFrame& frame)
    {
        const auto centerX = options.width / 2;
        const auto centerY = options.height / 2;
        const auto radiusX = std::max(90, options.width / 4);
        const auto radiusY = std::max(48, options.height / 5);

        const auto pointCount = frame.orbits.empty() ? 96 : std::max<std::size_t>(frame.orbits.front().points.size(), 24);
        for (std::size_t i = 0; i < pointCount; ++i)
        {
            const auto angle = (static_cast<double>(i) / static_cast<double>(pointCount)) * 2.0 * Pi;
            const auto x = centerX + static_cast<int>(std::cos(angle) * radiusX);
            const auto y = centerY + static_cast<int>(std::sin(angle) * radiusY);
            clearRect(x, y, 3, 2, 0.18f, 0.55f, 0.85f);
        }
    }

    void drawBody(const protocol::SceneFrame& frame)
    {
        int offsetX = 0;
        int offsetY = 0;
        if (!frame.bodies.empty())
        {
            offsetX = static_cast<int>(frame.bodies.front().transform[12] * 12.0);
            offsetY = static_cast<int>(frame.bodies.front().transform[13] * 12.0);
        }

        const auto size = std::max(34, std::min(options.width, options.height) / 10);
        const auto x = options.width / 2 - size / 2 + offsetX;
        const auto y = options.height / 2 - size / 2 + offsetY;

        clearRect(x - 4, y - 4, size + 8, size + 8, 0.06f, 0.12f, 0.22f);
        clearRect(x, y, size, size, 0.25f, 0.52f, 0.95f);
        clearRect(x + size / 5, y + size / 5, size / 2, size / 3, 0.18f, 0.72f, 0.42f);
        clearRect(x + size / 2, y + size / 2, size / 3, size / 3, 0.92f, 0.92f, 0.82f);
    }

    protocol::ViewInputEvent inputEvent(std::string device, std::string action)
    {
        protocol::ViewInputEvent input;
        input.sequence = ++inputSequence;
        input.device = std::move(device);
        input.action = std::move(action);
        return input;
    }

    std::string modifiers(SDL_Keymod keymod)
    {
        std::string result;
        if ((keymod & KMOD_SHIFT) != 0)
            result += "Shift";
        if ((keymod & KMOD_CTRL) != 0)
            result += result.empty() ? "Ctrl" : "+Ctrl";
        if ((keymod & KMOD_ALT) != 0)
            result += result.empty() ? "Alt" : "+Alt";
        return result;
    }

    View3DLoopOptions options;
    SDL_Window* window{ nullptr };
    SDL_GLContext context{ nullptr };
    bool sdlInitialized{ false };
    bool closeRequested{ false };
    std::uint64_t renderedFrames{ 0 };
    std::uint64_t inputSequence{ 0 };
    std::vector<protocol::ViewInputEvent> pendingInput;
    std::string rendererInfo;
};

View3DLoop::View3DLoop()
    : impl_(std::make_unique<Impl>())
{
}

View3DLoop::~View3DLoop()
{
    stop();
}

bool
View3DLoop::start(const View3DLoopOptions& options, std::string* errorMessage)
{
    return impl_->start(options, errorMessage);
}

bool
View3DLoop::render(const protocol::SceneFrame& frame, std::string* errorMessage)
{
    return impl_->render(frame, errorMessage);
}

std::vector<protocol::ViewInputEvent>
View3DLoop::drainInputEvents()
{
    return impl_->drainInputEvents();
}

void
View3DLoop::stop()
{
    impl_->stop();
}

bool
View3DLoop::isRunning() const
{
    return impl_->isRunning();
}

std::uint64_t
View3DLoop::renderedFrameCount() const
{
    return impl_->renderedFrames;
}

std::string
View3DLoop::rendererInfo() const
{
    return impl_->rendererInfo;
}

} // namespace celestia::runtime::view3d
