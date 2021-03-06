// ==================== LOCAL INCLUDES ====================

#include "LevelScreen.h"

// ==================== LEVEL SCREEN CLASS ====================

// ==================== CONSTRUCTOR / DESTRUCTOR ====================

LevelScreen::LevelScreen(Controller *_controller) : controller(_controller) {
    
    // Set the default attributes
	width = 0;
	height = 0;
	tileWidth = 0;
	tileHeight = 0;
	firstTileID = 0;
    
    // Create the viewport
    viewport = sf::View(sf::FloatRect(0, 0, LEVEL_SCREEN_VIEWPORT_WIDTH, LEVEL_SCREEN_VIEWPORT_HEIGHT));
    
}

// ==================== LOAD LEVEL ====================

bool LevelScreen::LoadFromFile(std::string filename) {

    // ==================== LOAD LEVEL FILE ====================

	// Load the level .tmx file using the TinyXML library
	TiXmlDocument levelFile(filename.c_str());
	if (!levelFile.LoadFile()) {
        // If something went wrong, return false to indicate level was not created
		std::cout << "Loading level \"" << filename << "\" failed."	<< std::endl;
		return false;
	}

    // Find the root map element
	TiXmlElement *map;
	map = levelFile.FirstChildElement("map");

	// Get the overall with & height in tiles as well as the width & height of each tile
	width = atoi(map->Attribute("width"));
	height = atoi(map->Attribute("height"));
	tileWidth = atoi(map->Attribute("tilewidth"));
	tileHeight = atoi(map->Attribute("tileheight"));

    // Find the first tile ID in the liteset element
	TiXmlElement *tilesetElement;
	tilesetElement = map->FirstChildElement("tileset");
	firstTileID = atoi(tilesetElement->Attribute("firstgid"));

	// Get the path to the tileset file image
	TiXmlElement *image;
	image = tilesetElement->FirstChildElement("image");
	std::string imagepath = image->Attribute("source");

	//std::string imagepath = "images/Level1Final.png";
	//std::cout << image->Attribute("source") << std::endl;

    // ==================== LOAD TILESHEET ====================
    
    // Load the tilesheet
	sf::Image img;
	if (!img.loadFromFile(imagepath)) {
        // If something went wrong, return false to indicate level was not created
		std::cout << "Failed to load tile sheet." << std::endl;
		return false;
	}
    
    // Load the tilesheet into the inage and set it as the tilesheet image
	img.createMaskFromColor(sf::Color(255, 255, 255));
	tilesetImage.loadFromImage(img);
	tilesetImage.setSmooth(false);

	// Calculate the number of rows and columns in the level
	int columns = tilesetImage.getSize().x / tileWidth;
	int rows = tilesetImage.getSize().y / tileHeight;

	// Construct a vector of rectangles representing the regions of each tile
	std::vector<sf::Rect<int>> subRects;
	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < columns; x++) {
			sf::Rect<int> rect;
			rect.top = y * tileHeight;
			rect.height = tileHeight;
			rect.left = x * tileWidth;
			rect.width = tileWidth;
			subRects.push_back(rect);
		}
	}

    // ==================== LOAD TILES ON EACH LAYER ====================
    
    // Get the first layer
	TiXmlElement *layerElement;
	layerElement = map->FirstChildElement("layer");
    
    // Repeat for each layer
	while (layerElement) {
        
        // The current layer object which will hold all the tiles in this layer
		Layer layer;

		// If there is an opacity attribute, apply it to the layer
		if (layerElement->Attribute("opacity") != NULL) {
			float opacity = strtod(layerElement->Attribute("opacity"), NULL);
			layer.opacity = 255 * opacity;
		} 
        // Otherwise, it is opaque by default
        else {
			layer.opacity = 255;
		}

		// Get the layer data
		TiXmlElement *layerDataElement;
		layerDataElement = layerElement->FirstChildElement("data");
    
        // If the layer contains no data 
		if (layerDataElement == NULL) {
            // Return false to indicate level was not created
            std::cout << "Bad map. No layer information found." << std::endl;
            return false;
		}
        
        // The textual data representing the all tiles on this layer
		const char* tileData = layerDataElement->GetText();
    
        // The current x & y coordinates of the current tile
		int x = 0;
		int y = 0;

		// Convert textual data representing the all tiles on this layer to a string stream
		std::string tileDataString(tileData);
		std::stringstream tileDataStringStream(tileDataString);
        
        // Read in each tile id as in integer, separated by commas into the tile data vector
		std::vector<int> tileDataVector;
		unsigned int currentTileID;
		while (tileDataStringStream >> currentTileID) {
            // Add the current tile id to the vector of tile ids
			tileDataVector.push_back(currentTileID);
            // Ignore all the commas and spaces while reading in the numbers
			if (tileDataStringStream.peek() == ',' || tileDataStringStream.peek() == ' ') {
				tileDataStringStream.ignore();
            }
		}
        
        // Loop through all the tiles by tile id
		for (currentTileID = 0; currentTileID < tileDataVector.size(); currentTileID++) {
            
            // Get the current tile id and determine which sub rectangle of the tileset to use for it
			int tileGID = tileDataVector.at(currentTileID);
			int subRectToUse = tileGID - firstTileID;

			// Set the texture rectangle of each tile if the sub rectangle of the tileset to use for it is valid
			if (subRectToUse >= 0) {
                
                // Create the sprite
				sf::Sprite sprite;
                
                // Set its image, position, and transparency
				sprite.setTexture(tilesetImage);
				sprite.setTextureRect(subRects[subRectToUse]);
				sprite.setPosition(x * tileWidth, y * tileHeight);
				sprite.setColor(sf::Color(255, 255, 255, layer.opacity));

                // Add it to the list of tiles for the current layer
				layer.tiles.push_back(sprite);
                
                //std::cout << "Added tile with subrect " << subRectToUse << " at position " << x << ", " << y << std::endl;
			}
            
            // Increment the tiles by X coordinate (column)
			x++;
            
            // If the end of the current row is reached
			if (x >= width) {
                
                // Move on the the start of the next row
				x = 0;
				y++;
                    
                //  Make sure the row staws within bounds
				if (y >= height) {
					y = 0;
                }
                
			}
            
		}
        
        // Add the current layer to the list of layers
		layers.push_back(layer);

        // Move on to the next layer
		layerElement = layerElement->NextSiblingElement("layer");
        
	} 

    // ==================== LOAD OBJECTS ON OBJECTGROUP LAYER ====================
    
    // If there is an objects layer
	TiXmlElement *objectGroupElement;
	if (map->FirstChildElement("objectgroup") != NULL) {

        // Find the objectgroup element which contains all the objects
		objectGroupElement = map->FirstChildElement("objectgroup");

        // Repeat for all the objects in the object group
		while (objectGroupElement) {
            
			//Get the current object
			TiXmlElement *objectElement;
			objectElement = objectGroupElement->FirstChildElement("object");

            // While there is a valid object
			while (objectElement) {
                
				// Get all the object data - type, name, position, etc...
				std::string objectType;
				std::string objectName;
				if (objectElement->Attribute("name") != NULL) {
					objectName = objectElement->Attribute("name");
				}
				int x = atoi(objectElement->Attribute("x"));
				int y = atoi(objectElement->Attribute("y"));

                // The width and height of the current object
				int width, height;

                // Set the image and position of the current object
				sf::Sprite sprite;
				sprite.setTexture(tilesetImage);
				sprite.setTextureRect(sf::Rect<int>(0, 0, 0, 0));
				sprite.setPosition(x, y);
                
                // If the current object has a width, load its wedth and height
				if (objectElement->Attribute("width") != NULL) {
					width = atoi(objectElement->Attribute("width"));
					height = atoi(objectElement->Attribute("height"));
				} 
                // Otherwise, this object's witdh and height is the width and height of the tile at this position
                // In this case, the object also has a sprite equal to the rectangle of the tile at this position
                else {
					width = subRects[atoi(objectElement->Attribute("gid")) - firstTileID].width;
					height = subRects[atoi(objectElement->Attribute("gid")) - firstTileID].height;
					sprite.setTextureRect(subRects[atoi(objectElement->Attribute("gid")) - firstTileID]);
				}

				// Create the object with the current attributes
				Object object;
				object.name = objectName;
				object.type = objectType;
				object.sprite = sprite;
                
                // Set the rectangle to define what position the object is in and its size
				sf::Rect<float> objectRect;
				objectRect.top = y;
				objectRect.left = x;
				objectRect.height = height;
				objectRect.width = width;
				object.rect = objectRect;

				// Find all the object properties
				TiXmlElement *properties;
				properties = objectElement->FirstChildElement("properties");
                
                // If this object has any special properties
				if (properties != NULL) {
                    
                    // Get the preperty
					TiXmlElement *prop;
					prop = properties->FirstChildElement("property");
                    
                    // If there is a valid preperty
					if (prop != NULL) {
                        
                        // Loop through all the properties
						while (prop) {
                            
                            // Get the preperty name and value
							std::string propertyName = prop->Attribute("name");
							std::string propertyValue = prop->Attribute("value");
                            
                            // Add it to the current object's list of preperties
							object.properties[propertyName] = propertyValue;
                            
                            // Move on to the next property
							prop = prop->NextSiblingElement("property");
                            
						}
                        
					}
                    
				}

                // Add the current object to the list of objects
				objects.push_back(object);

                // Move on to the next object
				objectElement = objectElement->NextSiblingElement("object");
			}
            
            // Move on to the next object group
			objectGroupElement = objectGroupElement->NextSiblingElement("objectgroup");
            
		}
	}
    
    // Otherwise, there was no object layers
	else {
		std::cout << "No object layers found..." << std::endl;
	}

    // Return true to indicate level was created
	return true;
    
}

// ==================== GETTERS ====================

Object LevelScreen::GetObject(std::string name) {
    
	// Return only the first object with the name passed in
    for (unsigned int i = 0; i < objects.size(); i++) {
        if (objects[i].name == name) {
            return objects[i];
        }
    }
    
    // If object is not found, return an empty
    return Object();
}

std::vector<Object> LevelScreen::GetObjects(std::string name) {
	
    // Return all the objects with the name passed in
	std::vector<Object> vec;
    for(unsigned int i = 0; i < objects.size(); i++) {
        if(objects[i].name == name) {
			vec.push_back(objects[i]);
        }
    }
    
    // Return the vector of matched objects
	return vec;
}

std::vector<Object> LevelScreen::GetAllObjects() {
   return objects;
}

sf::Vector2i LevelScreen::GetTileSize() {
	return sf::Vector2i(tileWidth, tileHeight);
}

// ==================== DRAW ====================

void LevelScreen::setViewport(sf::RenderWindow &window, sf::Vector2f &centerPosition, bool forceFullscreen) {
    
    bool codingWindowState = controller->getCodingWindowState();
    bool pauseWindowState = controller->getPauseWindowState();
    
    //std::cout << "Current: " << codingWindowState <<  " Prev: " << prev_CodingWindowState << std::endl;
            
    // If the coding window is curretly open and the pause window is not
    if((codingWindowState && !pauseWindowState) || forceFullscreen) {
        // Set the viewport to be fullscreen since that is reqiured for the proper operation of the coding window
        viewport.reset(sf::FloatRect(0, 0, WIDTH, HEIGHT));
    } 
    // Otherwise
    else {
        
        // When the coding window is closed, set the viewport back to a limited one
        viewport.reset(sf::FloatRect(0, 0, LEVEL_SCREEN_VIEWPORT_WIDTH, LEVEL_SCREEN_VIEWPORT_HEIGHT));

    
        // Calculate bounded center position
        float x = std::min((float)std::max((float)centerPosition.x, (float)(LEVEL_SCREEN_VIEWPORT_WIDTH / 2)), (float)((width * tileWidth) - (LEVEL_SCREEN_VIEWPORT_WIDTH / 2)));
        float y = std::min((float)std::max((float)centerPosition.y, (float)(LEVEL_SCREEN_VIEWPORT_HEIGHT / 2)), (float)((height * tileHeight) - (LEVEL_SCREEN_VIEWPORT_HEIGHT / 2)));
        
        // Set the center positoin of the viewport
        viewport.setCenter(x, y);
    
    }
    
    // Apply the viewport to the window
    window.setView(viewport);
    
}

void LevelScreen::draw(sf::RenderWindow &window) {
    
	// Draw all the tiles on all the levels (not the objects)
	for(unsigned int layer = 0; layer < layers.size(); layer++) {
		for(unsigned int tile = 0; tile < layers[layer].tiles.size(); tile++) {
			window.draw(layers[layer].tiles[tile]);
        }
    }
    
    /*
    // [Testing] Draw all the objects as outlines
    for (unsigned int i = 0; i < objects.size(); i++) {

        sf::RectangleShape rectShape;
        rectShape.setSize(sf::Vector2f(objects[i].rect.width, objects[i].rect.height));
        rectShape.setOutlineThickness(1);
        rectShape.setFillColor(sf::Color::Transparent);
        rectShape.setOutlineColor(sf::Color::Red);
        rectShape.setPosition(sf::Vector2f(objects[i].rect.left, objects[i].rect.top));

        window.draw(rectShape);

    }
    */
    
}

// ==================== OBJECT CLASS ====================

// ==================== GETTERS ====================

int Object::GetPropertyInt(std::string name) {
    return atoi(properties[name].c_str());
}

float Object::GetPropertyFloat(std::string name) {
    return strtod(properties[name].c_str(), NULL);
}

std::string Object::GetPropertyString(std::string name) {
    return properties[name];
}
