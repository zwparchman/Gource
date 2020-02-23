#pragma once

#include <vector>
#include <string>
#include "pawn.h"

class Toast{
    struct element {
        element(std::string str, float toastTime);
        std::string str;
        float toastTime;
        float elapsed;
        float getAlpha();
        bool isFinished() const { return toastTime < elapsed; }
        void step(float dt) { elapsed += dt; }
    };

    std::vector<element> toasts;
public:
    void draw(float dt, vec2 location, float scale, FXFont &font);
    void addToast(const std::string & str, float toastTime);
};
