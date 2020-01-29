/*
    Copyright (C) 2009 Andrew Caudwell (acaudwell@gmail.com)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef RACTION_H
#define RACTION_H

#include "user.h"
#include "file.h"
#include "functional"

class RUser;
class RFile;

class RAction {
public:
    vec3 colour;
    void apply();
    RUser* source;
    RFile* target;

    time_t timestamp;
    float t;

    float progress;
    float rate;

    RAction(RUser* source, RFile* target, time_t timestamp, float t, const vec3& colour);
    ~RAction() {};

    std::optional<std::function<void(RAction &, float, float)>> OnLogic;
    std::optional<std::function<void(RAction &)>> OnApply;
    
    inline bool isFinished() const { return (progress >= 1.0); };

    void logic(float dt);

    void drawToVBO(quadbuf& buffer) const ;
    void draw(float dt);

    static RAction CreateAction(RUser* source, RFile* target, time_t timestamp, float t);
    static RAction RemoveAction(RUser* source, RFile* target, time_t timestamp, float t);
    static RAction ModifyAction(RUser* source, RFile* target, 
                                time_t timestamp, float t, const vec3& modify_colour);
};


#endif

