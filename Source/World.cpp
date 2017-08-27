#include "World.h"

#include <iostream>
#include <iomanip>

#include "Random.h"
#include "Common.h"
#include "Config.h"



World::World(const Config& config)
:   m_people    (config.width * config.height)
,   m_pConfig   (&config)
{
    m_worldTexture.loadFromImage(config.image);
    m_world.setTexture  (&m_worldTexture);
    m_world.setSize     ({(float)config.width,
                          (float)config.height});
    createColonies();
    initText();
}

void World::update()
{
    std::vector<Person> newPeople(m_pConfig->width * m_pConfig->height);

    for (auto& c : m_colonyStats)
    {
        c.members = 0;
        c.strength = 0;
    }

    for (unsigned y = 0; y < m_pConfig->height; y++)
    for (unsigned x = 0; x < m_pConfig->width; x++)
    {
        auto& person    = m_people[getIndex(m_pConfig->width, x, y)];
        auto& stats   = m_colonyStats[person.getData().colony];

        if (!person.getData().isAlive)
            continue;
        person.update();
        if (!person.getData().isAlive)
            continue;


        //Get new location to move to
        int xMoveTo = x + Random::get().intInRange(-1, 1);
        int yMoveTo = y + Random::get().intInRange(-1, 1);
        if (xMoveTo < 0 || xMoveTo >= (int)m_pConfig->width) continue;
        if (yMoveTo < 0 || yMoveTo >= (int)m_pConfig->height) continue;

        //Store this for the stats to use at the end of the loop
        auto  strength      = person.getData().strength;
        auto& movePerson    = m_people[getIndex(m_pConfig->width, xMoveTo, yMoveTo)];

        //If trying to move onto water or onto square where person of same colony is
        //, stay put
        if (isWater(xMoveTo, yMoveTo))
        {
            stats.strength    += strength;
            stats.members     ++;
            newPeople[getIndex(m_pConfig->width, x, y)] = person;
            continue;
        }
        else if (movePerson.getData().colony == person.getData().colony)
        {
            stats.strength  += strength;
            stats.members ++;

            if (movePerson.getData().isDiseased)
            {
                person.giveDisease();
            }

            newPeople[getIndex(m_pConfig->width, x, y)] = person;
            continue;
        }


        //Try move to new spot
        //Fight other person if need be
        if (movePerson.getData().colony != person.getData().colony)
        {
            if (movePerson.getData().isAlive)
            {
                person.fight(movePerson);
                if (!person.getData().isAlive)
                {
                    continue;
                }
            }
        }
        //if the fight is survived, then good news!
        newPeople[getIndex(m_pConfig->width, xMoveTo, yMoveTo)] = person;

        //try give birth
        if (person.getData().productionCount >= REPRODUCE_THRESHOLD)
        {
            //The person itself has moved to a new spot, so it is ok to mess with it's data now
            person.init(person.getChild());
        }
        else
        {
            //Kill the old person, the current person has now moved
            person.kill();
        }

        //This will either be a dead person, or a newborn
        newPeople[getIndex(m_pConfig->width, x, y)] = person;


        //Finally, do stuff for the stats
        stats.members ++;
        stats.strength  += strength;
    }
    m_people = std::move(newPeople);
}

const sf::Color& World::getColorAt(unsigned x, unsigned y) const
{
    return m_colonies[m_people[getIndex(m_pConfig->width, x, y)].getData().colony].colour;
}

bool World::isGrass(unsigned x, unsigned y) const
{
    return m_pConfig->image.getPixel(x, y).g > 235;
}

bool World::isWater(unsigned x, unsigned y) const
{
    return m_pConfig->image.getPixel(x, y).b > 235;
}

void World::draw(sf::RenderWindow& window) const
{
    window.draw(m_world);
}

void World::drawText(sf::RenderWindow& window)
{
    for (auto& stats : m_colonyStats)
    {
        std::ostringstream stream;

        int averageStr = abs((stats.members > 0) ?
            stats.strength / stats.members :
            0);


        stream  << std::left
                << std::setw(10)    << stats.name
                << std::setw(7)     << stats.members
                << std::setw(10)    << " Avg Str: " << averageStr << '\n';

        stats.text.setString(stream.str());
        window.draw(stats.text);
    }
}

void World::createColonies()
{
    ColonyCreator creator(m_pConfig->image);

    auto locations   = creator.createColonyLocations(m_pConfig->width, m_pConfig->height);
    m_colonies  = creator.createColonyStats();

    //Place colonies at the locations
    for (unsigned i = 1; i < NUM_COLONIES; i++)
    {
        auto& location = locations[i];
        //place up to 50 people at the location
        for (int j = 0; j < 50; j++)
        {
            int xOffset = Random::get().intInRange(-4, 4);
            int yOffset = Random::get().intInRange(-4, 4);

            int newLocationX = xOffset + location.x;
            int newLocationY = yOffset + location.y;

            if (newLocationX < 0 || newLocationX >= (int)m_pConfig->width) continue;
            if (newLocationY < 0 || newLocationY >= (int)m_pConfig->height) continue;
            if (isWater(newLocationX, newLocationY)) continue;

            PersonData data;
            data.age        = 0;
            data.strength   = Random::get().intInRange(500, 600);
            data.isAlive    = true;
            data.colony     = i;

            m_people[getIndex(m_pConfig->width, newLocationX, newLocationY)].init(data);

        }
    }
}

void World::initText()
{
    int charSize = 17;
    m_statsFont.loadFromFile("res/arial.ttf");

    for (int i = 0; i < NUM_COLONIES; i++)
    {
        auto& stats = m_colonyStats[i];
        stats.name = "Colony " + std::to_string(i) + ": ";
        stats.text.setCharacterSize(charSize);
        stats.text.move(10, m_pConfig->height - i * charSize - 30);
        stats.text.setOutlineColor(sf::Color::Black);
        stats.text.setFillColor(m_colonies[i].colour);
        stats.text.setOutlineThickness(1);
        stats.text.setFont(m_statsFont);
    }
}























