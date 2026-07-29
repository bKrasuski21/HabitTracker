#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class HabitSquare
{
    public:

    HabitSquare(int w_loc, int h_loc, int color = 0, int square_num = 0);
    void draw(sf::RenderWindow& window);
    void onClick(sf::Vector2f mousePos, sf::Mouse::Button clickType);
    int getClicked(){return clicked;}
    void colorCommander(int color_val);

    private:
    std::vector<sf::Color> colors{sf::Color::Black, sf::Color::Green, sf::Color::Yellow};
    sf::RectangleShape square;
    int width_loc;
    int height_loc;
    int clicked = 0;
    float squareSize = 55;
    int square_date = 0;
    sf::Color squareColor{255,255,255};
    sf::Color squareOutline{0,0,0};
    sf::Text square_date_text;

};