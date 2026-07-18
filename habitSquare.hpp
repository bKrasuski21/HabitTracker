#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class HabitSquare
{
    public:
    HabitSquare(int w_loc, int h_loc, bool green = false);
    void draw(sf::RenderWindow& window);
    void onClick(sf::Vector2f mousePos);
    bool getClicked(){return clicked;}

    private:
    sf::RectangleShape square;
    int width_loc;
    int height_loc;
    bool clicked = false;
    float squareSize = 55;
    sf::Color squareColor{255,255,255};
    sf::Color squareOutline{0,0,0};

};