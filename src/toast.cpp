#include "toast.h"
#include "file.h"
#include <algorithm> 


extern FXFont file_font;

Toast::element::element(std::string str, float toastTime)
    :str(str), toastTime(toastTime), elapsed(0.0f)
{
}
float Toast::element::getAlpha(){
    return std::min((toastTime / elapsed / 2.0f ), 1.0f);
}

void Toast::draw(float dt, vec2 location, float scale, FXFont& font){
    auto split = std::remove_if(toasts.begin(), toasts.end(), [](element &t) { return t.isFinished(); });
    toasts.erase(split, toasts.end());

    vec2 curPos = location;
    for(int i=0; i < 10 && i < toasts.size() ; i++){
        element &td = toasts[i];
        td.step(dt);

        curPos.y += 20;
        font.setAlpha(td.getAlpha());
        font.draw(curPos.x, curPos.y, td.str);
    }

}

void Toast::addToast(const std::string & str, float toastTime){
    toasts.emplace_back(str, toastTime);
}
