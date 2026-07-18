#include "nameBlock.hpp"

NameBlock::NameBlock(std::string name, int height)
:
habit_name(name),
height_loc(height)
{

}   

void NameBlock::initialize(){
    if(!this->habit_font.loadFromFile("font.ttf")){
        return;
    }   
    this->habit_text.setFont(this->habit_font);
    this->habit_text.setString(this->habit_name);
    this->habit_text.setCharacterSize(50);
    this->habit_text.setFillColor(sf::Color::White);
    this->habit_text.setPosition(this->width_loc, this->height_loc);
}   

void NameBlock::draw(sf::RenderWindow& window){
    window.draw(this->habit_text);

}