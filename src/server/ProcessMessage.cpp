#include "servermain.h"

void ProcessMessage(PDATA pdata)
{
    GameObject* gameObject;
    
    // ***** lock mutex
    pthread_mutex_lock(pdata->lock);
    Server*  server = pdata->server;
    // unlock mutex *****
    pthread_mutex_unlock(pdata->lock);

    Message* recvMessage;
    Message  sendMessage;
    Point pt;
    int clientID;
    std::string data;
    std::string playerName;
    std::istringstream istr;
    std::ostringstream ostr;
    std::map<int, GameObject*>::iterator ii;

    // object creation parameters
    int type, objID, playerID, speed, health, attack;
    double degree, posX, posY, maxSpeed;
    
    while(pdata->isRunning && (recvMessage = server->read()))
    {
        clientID = recvMessage->getID();
        sendMessage.setID(clientID);
        
        // ***** lock mutex
        pthread_mutex_lock(pdata->lock);
        
        switch(recvMessage->getType())
        {
        case Message::CONNECTION:
            // extract ship type and player name from message data
            istr.clear();
            istr.str(recvMessage->getData());
            istr >> playerName;
            
            // check if max player is reached on server
            if(pdata->ships.size() >= 8)
            {
                data = std::string("Refused");
                if(sendMessage.setAll(data, Message::CONNECTION))
                    server->write(&sendMessage, clientID);
                break;
            }

            data = std::string("Accepted");
            // add new client to the clients map
            if(pdata->clients.count(clientID) > 0)
            {
                pdata->clients.erase(clientID);
            }
            pdata->clients[clientID] = playerName;
            
            printf("client (ID: %d) connected\n", clientID);

            if(sendMessage.setAll(data, Message::CONNECTION))
                server->write(&sendMessage, clientID);
            
            // send CREATION message for every object in the maps to the client
            for(ii = pdata->projectiles.begin(); ii != pdata->projectiles.end(); ++ii)
            {
                sendMessage.setID(((GOM_Ship*)ii->second)->getPlayerID());
                sendMessage.setAll(ii->second->toString(), Message::CREATION);
                server->write(&sendMessage, clientID);
                
                // debugging
                std::cout << ii->second->toString() << std::endl;
            }
            
            for(ii = pdata->ships.begin(); ii != pdata->ships.end(); ++ii)
            {
                sendMessage.setID(((GOM_Projectile*)ii->second)->getPlayerID());
                sendMessage.setAll(ii->second->toString(), Message::CREATION);
                server->write(&sendMessage, clientID);
                
                // debugging
                std::cout << ii->second->toString() << std::endl;
            }
            break;

        case Message::RESPAWN:
            // extract ship type from msg data
            istr.clear();
            istr.str(recvMessage->getData());
            istr >> type;

	    // get a furthest start point from other ships
            pt = getStartPoint(pdata->ships);    
            objID    = pdata->objCount++;
	    degree   = 0;
            posX     = pt.getX();
            posY     = pt.getY();
            //posX = 400 * (pdata->ships.size()+1);
            //posY = 500;
	    playerID = clientID;
            speed    = 0;
	    
	    switch(type)
	    {
	      case 0:    //Galeon
		health   = 100;     
		attack   = 30;   
		maxSpeed = 3.6;
		break;
		
	      case 1:   //Battleship
		health   = 200;     
		attack   = 50;
		maxSpeed = 2;
		break;   
		
	      case 2:   //Dingy
		health   = 5;     
		attack   = 200;
		maxSpeed = 6;
		break;
	    }

            // create the GOM_Ship object
            gameObject = new GOM_Ship(ObjectType(type), objID, degree, posX, posY,
                                   playerID, speed, health, attack, maxSpeed);

            // add the game object to the map
            if(pdata->ships.count(clientID) > 0)
            {
                delete pdata->ships[clientID];
                pdata->ships.erase(clientID);
            }
            pdata->ships[clientID] = gameObject;

            sendMessage.setID(clientID);

            //Send CREATION message to all clients
            if(sendMessage.setAll(gameObject->toString(), Message::CREATION))
            {
                server->write(&sendMessage);

                // debugging
                std::cout << "ship string for client#" << clientID << std::endl;
                std::cout << pdata->ships[clientID]->toString() << std::endl;
            }
            else
            {
                printf("ERROR(client ID: %d): unable to echo CREATION message\n");
            }
            break;

        case Message::ACTION:
            // extract projectile info from msg data
            istr.clear();
            istr.str(recvMessage->getData());
            istr >> posX >> posY >> degree;

            // create a projectile object
            objID = pdata->objCount++;
            gameObject = new GOM_Projectile(PROJECTILE, objID, degree, posX, posY,
                                            clientID, 9, 50, 10);

            // debugging
            //std::cout << "projectile (clientID:" << clientID << ") - ";
            //std::cout << gameObject->toString() << std::endl;
            //std::cout << "recved data: " << recvMessage->getData() << std::endl;
            
            // add projectile object to the projectil map
            if(pdata->projectiles.count(objID) > 0)
            {
                delete pdata->projectiles[objID];
                pdata->projectiles.erase(objID);
            }
            pdata->projectiles[objID] = gameObject;

            //Send CREATION message to all clients
            if(sendMessage.setAll(gameObject->toString(), Message::CREATION))
            {
                server->write(&sendMessage);
            }
            else
            {
                printf("ERROR(client ID: %d): unable to echo ACTION message\n");
            }
            break;

        case Message::DEATH:
            // send DELETION msg to all clients
            ostr.clear();
            ostr.str("");

            if(pdata->ships.count(clientID) > 0)
            {
                ostr << "S " << pdata->ships[clientID]->getObjID() << " 1";
                sendMessage.setAll(ostr.str(), Message::DELETION);
                server->write(&sendMessage);

                delete pdata->ships[clientID];
                pdata->ships.erase(clientID);
            }
            break;

        case Message::UPDATE:
            data = recvMessage->getData();
            
            // retrieve object's ID from the data string
            if((objID = GameObjectFactory::getObjectID(data)) == -1)
			{
				std::cout << "updated failed" << std::endl;
                break;
			}
            
            // update only if the object exists
            if(pdata->ships.count(clientID) > 0)
                pdata->ships[clientID]->update(data);

            //pdata->ships[objID]->printHitBox(std::cout);
            
            // echo the UPDATE message to all clients
            // fall through

        case Message::CHAT:
            //echo to all clients
            sendMessage = *recvMessage;
            server->write(&sendMessage);
            break;
        }// end of swtich()
        
        // unlock mutex *****
        pthread_mutex_unlock(pdata->lock);

        delete recvMessage;
    }// end of while()
}
