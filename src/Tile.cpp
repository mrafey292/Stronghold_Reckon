#include "Tile.hpp"
#include "Building.hpp"
#include "TextureManager.hpp"
#include "IsometricUtils.hpp"
#include <iostream>
#include <random>

// Constructor
Tile::Tile(int row, int col, TileType type)
    : type(type), row(row), col(col), blockStatus(false), building(nullptr), tower(nullptr) {
    updateTexture();
}

// Copy Constructor: Remove if deep copying isn't needed
Tile::Tile(const Tile& other)
    : type(other.type), row(other.row), col(other.col), blockStatus(other.blockStatus),
      texturePath(other.texturePath), building(other.building), tower(other.tower), // Use references
      health(other.health), grassTileIndex(other.grassTileIndex) {
    if (type == TileType::Grass) {
        loadTexture();
    } else {
        TextureManager& tm = TextureManager::getInstance();
        auto texturePtr = tm.getTexture(texturePath);
        if (texturePtr) {
            sprite.setTexture(*texturePtr);
            sprite.setScale(
                static_cast<float>(TILE_WIDTH) / static_cast<float>(texturePtr->getSize().x),
                static_cast<float>(TILE_HEIGHT) / static_cast<float>(texturePtr->getSize().y)
            );
        }
    }
    sprite.setOrigin(other.sprite.getOrigin());
    sprite.setPosition(other.sprite.getPosition());
    sprite.setScale(other.sprite.getScale());
}

// Setter for texturePath (only for non-grass tiles)
void Tile::setTexturePath(const std::string& path) {
    texturePath = path;
    loadTexture();
}

// Getter for texturePath
std::string Tile::getTexturePath() const {
    return texturePath;
}

// Helper method to load texture based on type and texturePath
void Tile::loadTexture() {
    TextureManager& tm = TextureManager::getInstance();
    std::shared_ptr<sf::Texture> texturePtr;

    switch (type) {
        case TileType::Grass: {
            texturePtr = tm.getSpriteSheet();
            if (!texturePtr) {
                std::cerr << "Sprite sheet not loaded.\n";
                return;
            }
            // Define sprite sheet grid
            const int columns = 3;
            const int rows = 6;
            const int totalTiles = columns * rows;
            const int tileWidth = 64;  // Adjust to your sprite sheet tile width
            const int tileHeight = 32; // Adjust to your sprite sheet tile height

            // If grassTileIndex is not set, randomly assign one
            if (grassTileIndex == -1) {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, totalTiles - 1);
                grassTileIndex = dis(gen);
            }

            // Calculate tile position in sprite sheet
            int tileX = (grassTileIndex % columns) * tileWidth;
            int tileY = (grassTileIndex / columns) * tileHeight;

            // Assign texture and texture rectangle
            sprite.setTexture(*texturePtr);
            sprite.setTextureRect(sf::IntRect(tileX, tileY, tileWidth, tileHeight));

            // Scale the sprite to fit the tile dimensions
            sprite.setScale(
                static_cast<float>(TILE_WIDTH) / static_cast<float>(tileWidth),
                static_cast<float>(TILE_HEIGHT) / static_cast<float>(tileHeight)
            );
            
            // sprite.setOrigin(static_cast<float>(TILE_WIDTH) / 2.0f, static_cast<float>(TILE_HEIGHT) / 2.0f);

            break;
        }

        case TileType::Water:
            texturePath = "../assets/tiles/water.png";
            texturePtr = tm.getTexture(texturePath);
            break;

        case TileType::Road:
            texturePath = "../assets/tiles/road.png";
            texturePtr = tm.getTexture(texturePath);
            break;

        case TileType::Wall:
            texturePath = "../assets/walls/brick_wall.png";
            texturePtr = tm.getTexture(texturePath);
            health = 100; // Initialize health for walls
            break;
        
        case TileType::Trap:
            texturePtr = tm.getTexture(trap->getTexturePath());
            break;

        case TileType::Tower:
            texturePtr = tm.getTexture(tower->getTexturePath());
            break;

    }

    // For non-grass tiles, set the texture and scale
    if (type != TileType::Grass && texturePtr) {
        sprite.setTexture(*texturePtr);
        sprite.setScale(
            static_cast<float>(TILE_WIDTH) / static_cast<float>(texturePtr->getSize().x),
            static_cast<float>(TILE_HEIGHT) / static_cast<float>(texturePtr->getSize().y)
        );
    } else if (type != TileType::Grass && !texturePtr) {
        std::cerr << "Failed to load texture for tile at (" << row << ", " << col << ")\n";
    }
}

// Set Type without altering building presence
void Tile::setType(TileType newType) {
    type = newType;
    // Reset grassTileIndex if changing to Grass
    if (type == TileType::Grass) {
        grassTileIndex = -1;
    }
    loadTexture();
    // Update blockStatus based on type and building presence
    if (building != nullptr) {
        blockStatus = true;
    } else {
        // Define which tile types are blocked
        switch (type) {
            case TileType::Water:
            case TileType::Trap:
            case TileType::Tower:
            case TileType::Wall:
                blockStatus = true;
                if (type == TileType::Wall) {
                    health = 100; // Reset health for walls
                }
                break;
            default:
                blockStatus = false;
                break;
        }
    }
}

TileType Tile::getType() const {
    return type;
}

void Tile::setBuilding(std::shared_ptr<Building> buildingPtr) {
    building = buildingPtr;
    if (buildingPtr) {
        // When a building is present, block the tile
        blockStatus = true;
    } else {
        // Re-evaluate blockStatus based on tile type
        switch (type) {
            case TileType::Water:
            case TileType::Wall:
                blockStatus = true;
                break;
            default:
                blockStatus = false;
                break;
        }
    }
}

std::shared_ptr<Building> Tile::getBuilding() const {
    return building;
}

void Tile::setPosition(float x, float y) {
    sprite.setPosition(x, y);
}

sf::Vector2f Tile::getPosition() const {
    return sprite.getPosition();
}

void Tile::updateTexture() {
    loadTexture();
}

int Tile::getRow() const {
    return row;
}

int Tile::getCol() const {
    return col;
}

void Tile::addEdge(std::shared_ptr<Tile> neighbor) {
    neighbors.push_back(neighbor);
}

const std::vector<std::shared_ptr<Tile>>& Tile::getNeighbors() const {
    return neighbors;
}

std::shared_ptr<Tile> Tile::getNeighbor(int dx, int dy) const {
    for (const auto& neighbor : neighbors) {
        if (neighbor->getRow() == row + dx && neighbor->getCol() == col + dy) {
            return neighbor;
        }
    }
    return nullptr;
}

bool Tile::isBlocked() const {
    return blockStatus;
}

bool Tile::isWall() const {
    return type == TileType::Wall;
}

void Tile::draw(sf::RenderWindow& window) const {
    window.draw(sprite);
    if (building) {
        building->draw(window, sprite.getPosition().x, sprite.getPosition().y);
    }
    if (tower) {
        tower->render(window);
    }
    if (trap) {
        trap->draw(window, sprite.getPosition().x, sprite.getPosition().y);
    }
    if (tower) {
        tower->render(window);
    }
}

void Tile::takeDamage(float damage) {
    if (isWall()) {
        health -= damage;
        // std::cout << "Wall at (" << row << ", " << col << ") takes " 
        //           << damage << " damage, remaining health: " << health << ".\n";

        if (health <= 0) {
            health = 0;
            blockStatus = false;
            type = TileType::Grass;
            building = nullptr;
            updateTexture();  // Update visual representation
            std::cout << "Wall at (" << row << ", " << col << ") destroyed.\n";
        }
    }
}

bool Tile::isDestroyed() const {
    return health == 0;
}

int Tile::getHealth() const {
    return health;
}

void Tile::setHealth(int healthValue) {
    health = healthValue;
}

void Tile::setBlockStatus(bool status) {
    blockStatus = status;
}

void Tile::setTower(std::shared_ptr<Tower> towerPtr) {
    // std::cout << "Tower placed at tile: (" << row << ", " << col << "). ptr: " << towerPtr << "\n";
    tower = towerPtr;
    blockStatus = (tower != nullptr);
}

std::shared_ptr<Tower> Tile::getTower() const {
    return tower;
}

void Tile::setGrassTileIndex(int index) {
    grassTileIndex = index;
    updateTexture(); // Trigger texture update with correct grass index
}


int Tile::getGrassTileIndex() const {
    return grassTileIndex;
}

// Traps

void Tile::setTrap(std::shared_ptr<Trap> trapPtr) {
    if (trapPtr) {
        // std::cout << "Trap placed at tile: (" << row << ", " << col << ").\n";
    }
    trap = trapPtr;
}

std::shared_ptr<Trap> Tile::getTrap() const {
    return trap;
}

// void Tile::setTrap(const std::string& trapTexture) {
//     this->trapTexture = trapTexture;
//     trapActive = true;
//     type = TileType::Trap;
//     loadTexture();
// }

bool Tile::hasTrap() const {
    return trap != nullptr;
}

// void Tile::triggerTrap() {
//     if (trapActive) {
//         std::cout << "Trap triggered at (" << row << ", " << col << ").\n";
//         trapActive = false;
//         type = TileType::Grass; // Reset to grass after triggering
//         updateTexture();
//     }
// }