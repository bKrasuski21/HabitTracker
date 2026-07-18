#include "habitSquare.hpp"


HabitSquare::HabitSquare(int square_w, int square_h, bool green) // probably pass in location data from row. 
:
square({this->squareSize, this->squareSize}),
width_loc(square_w),
height_loc(square_h),
clicked(green)
{
this->square.setPosition(width_loc, height_loc);
if(clicked){
this->square.setFillColor({0,128,0});
}else {
this->square.setFillColor({0,0,0});
}
this->square.setOutlineThickness(2.f);

}


void HabitSquare::draw(sf::RenderWindow& window){
    window.draw(this->square);
}

void HabitSquare::onClick(sf::Vector2f mousePos){
    if(this->square.getGlobalBounds().contains(mousePos)){
        if(!clicked){
            this->square.setFillColor({0,128,0});
            clicked = !clicked;
        }else if(clicked) {
            this->square.setFillColor({0,0,0});
            clicked = !clicked;
        }
        
    }
}