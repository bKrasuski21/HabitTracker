#include "habitSquare.hpp"


HabitSquare::HabitSquare(int square_w, int square_h, int color, int square_nums) // probably pass in location data from row. 
:
square({this->squareSize, this->squareSize}),
width_loc(square_w),
height_loc(square_h),
clicked(color),
square_date(square_nums)
{
this->square.setPosition(width_loc, height_loc);
this->square.setFillColor(colors[clicked]);
this->square.setOutlineThickness(2.f);

}


void HabitSquare::draw(sf::RenderWindow& window){
    window.draw(this->square);
}

void HabitSquare::onClick(sf::Vector2f mousePos, sf::Mouse::Button clickType){
    if(this->square.getGlobalBounds().contains(mousePos)){
        colorCommander((clickType==sf::Mouse::Left) ? 1 : 2);
    
    }
}



void HabitSquare::colorCommander(int color_val){
    if(!clicked){
        clicked = color_val;
        square.setFillColor(colors[clicked]);
    }else {
        clicked = 0;
        square.setFillColor(colors[clicked]);
    }  
}

