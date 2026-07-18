#pragma once
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>

class NameBlock{
    public:
    NameBlock(std::string name, int height); // still need to get height and set it to height_loc
    void initialize(); // create text block, set text block to name, set width and height locations.
    void draw(sf::RenderWindow& window);
    std::string getString() {return this->habit_text.getString();}
    
    private:
    int height_loc;
    int width_loc = 0;
    std::string habit_name;

    sf::Text habit_text;
    sf::Font habit_font; 
};