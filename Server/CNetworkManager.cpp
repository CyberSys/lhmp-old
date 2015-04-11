#include "CNetworkManager.h"
#include "CCore.h"
#include "CTCP.h"
#include <sstream>
#include "TCPInterface.h"

#include "../shared/limits.h"
#include "../shared/gamestructures.h"
#include "../shared/version.h"

#include "FileList.h"
#include "FileListTransfer.h"

extern CCore *g_CCore;

#include "../shared/linux.h"

DWORD postTime = timeGetTime();
CNetworkManager::CNetworkManager()
{
	peer = NULL;
	this->m_pServerMode = "Default+Mode";
}
CNetworkManager::~CNetworkManager()
{
	delete sd;
	RakNet::RakPeerInterface::DestroyInstance(peer);
	delete(slot);
}
void CNetworkManager::PostMasterlist(bool now){
	int port = this->m_pServerPort;
	CTCP *cTCP = new CTCP(port);
	TCPInterface TCP;
	if (TCP.Start(0, 0) != false)
	{
		SystemAddress server;
		server = TCP.Connect("lh-mp.eu", 80, true);
		if (server != UNASSIGNED_SYSTEM_ADDRESS)			// if we are connected
		{
			int maxplayers = this->m_pServerMaxPlayers;
			std::string svrname = this->m_pSvrName;
			for (size_t pos = svrname.find(' ');
				pos != std::string::npos;
				pos = svrname.find(' ', pos))
			{
				svrname.replace(pos, 1, "+");
			}

			char postRequest[2048] = "";
			char data[1024] = "";
			sprintf_s(data, "name=%s&pt=%d&ms=%d&mod=%s", svrname.c_str(), port, maxplayers, "");
			sprintf_s(postRequest,
				"POST /query/add.php HTTP/1.1\r\n"
				"Content-Type: application/x-www-form-urlencoded\r\n"
				"Content-Length: %d\r\n"
				"Host: lh-mp.eu\r\n\r\n"
				"%s\r\n", strlen(data), data);
			TCP.Send(postRequest, strlen(postRequest), server, false);
			RakNet::TimeMS StartTime = RakNet::GetTimeMS();
			while (1)
			{
				RakNet::Packet *packet = TCP.Receive();
				if (packet)
				{
					if (packet->data[13] == 'O'){
						printf("Server successfully posted to master server.\n");
					}
					else{
						printf("Posting to master server failed.\n");
					}
					break;
				}
				if (RakNet::GetTimeMS() - StartTime > 5000)
				{
					printf("Posting to master server failed.\n");
					break;
				}
				Sleep(100);
			}
		}
		else
		{
			printf("Connection failed");
		}
	}
}
void CNetworkManager::UpdateMasterlist(){
	TCPInterface TCP;
	if (TCP.Start(0, 0) == false)
		return;
	SystemAddress server;
	server = TCP.Connect("lh-mp.eu", 80, true);
	if (server != UNASSIGNED_SYSTEM_ADDRESS)			// if we are connected
	{
		std::string mod = this->m_pServerMode;
		for (size_t pos = mod.find(' ');
			pos != std::string::npos;
			pos = mod.find(' ', pos))
		{
			mod.replace(pos, 1, "+");
		}
		char postRequest[2048] = "";
		char data[1024] = "";
		sprintf_s(data, "port=%d&players=%d&mod=%s", this->m_pServerPort, peer->NumberOfConnections(),mod.c_str());
		sprintf_s(postRequest,
			"POST /query/update.php HTTP/1.1\r\n"
			"Content-Type: application/x-www-form-urlencoded\r\n"
			"Content-Length: %d\r\n"
			"Host: lh-mp.eu\r\n\r\n"
			"%s\r\n", strlen(data), data);
		TCP.Send(postRequest, strlen(postRequest), server, false);
		while (TCP.ReceiveHasPackets() == false)
			RakSleep(10);
		RakNet::Packet* pack = TCP.Receive();
	} else
	{
		printf("Connection to masterlist failed\n");
	}
#ifdef _WIN32
	char buff[255];
	sprintf(buff, "%s:%u Players: %d/%d", this->m_pSvrName.c_str(), this->m_pServerPort, peer->NumberOfConnections(),this->m_pServerMaxPlayers);
	SetConsoleTitle(buff);
#endif
}
bool CNetworkManager::Init(int port, int players,std::string startpos,std::string mode)
{
	m_pServerMaxPlayers = players;
	m_pServerPort		= port;
	m_pServerMode		= mode;
	peer	= RakNet::RakPeerInterface::GetInstance();
	sd		= new SocketDescriptor(port,0);
	slot = new Slot[players];
	for(int i  = 0; i < players; i++)
	{
		slot[i].sa = UNASSIGNED_SYSTEM_ADDRESS;
		slot[i].isUsed = false;
	}

	if (peer->Startup(100, sd, 1) != RAKNET_STARTED)
	{
		g_CCore->GetLog()->AddNormalLog("Startup failed !");
		return false;
	}
	peer->SetMaximumIncomingConnections(players);
	return true;
}
int CNetworkManager::GetIDFromSystemAddress(SystemAddress a)
{
	for(int i=0;i < m_pServerMaxPlayers;i++)
	{
		if (slot[i].isUsed == false)
			continue;
		if(slot[i].sa == a)
		{
			return i;
		}
	}
	return -1;
}

SystemAddress CNetworkManager::GetSystemAddressFromID(int i)
{
	return slot[i].sa;
}


Slot*		CNetworkManager::GetSlotID(int ID)
{
	return &this->slot[ID];
}

int	CNetworkManager::GetFirstFreeSlot()
{
	for(int i=0;i < m_pServerMaxPlayers;i++)
	{
		if(slot[i].isUsed == false)
			return i;
	}
	return -1;
}
void		CNetworkManager::SetServerName(std::string name)
{
	this->m_pSvrName = name;
}
std::string		CNetworkManager::GetServerName()
{
	return this->m_pSvrName;
}
void			CNetworkManager::SetMaxServerPlayers(int max)
{
	this->m_pServerMaxPlayers = max;
}
int				CNetworkManager::GetMaxServerPlayers()
{
	return this->m_pServerMaxPlayers;
}
void			CNetworkManager::SetServerMode(std::string mode)
{
	this->m_pServerMode = mode;
}
std::string		CNetworkManager::GetServerMode()
{
	return this->m_pServerMode;
}

void			CNetworkManager::SetWebsite(char* web)
{
	strcpy_s(this->website, 512,web);
}
char*			CNetworkManager::GetWebsite()
{
	return this->website;
}

// Returns the real count of players (doesn't include players who are downloading files)
unsigned int	CNetworkManager::GetPlayerCount()
{
	unsigned int count = 0;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (g_CCore->GetPlayerPool()->Return(i) != NULL)
		{
			count++;
		}
	}
	return count;
}
unsigned int	CNetworkManager::GetMaxPlayerCount()
{
	return this->GetPeer()->GetMaximumIncomingConnections();
}

void	CNetworkManager::OnPlayerConnection(RakNet::Packet* packet)
{
	int offset = 0;
	if (packet->data[0] == (MessageID)ID_TIMESTAMP)
		offset = 1 + sizeof(RakNet::TimeMS);

	BitStream bsIn(packet->data + offset, packet->length - offset, false);
	bsIn.IgnoreBytes(sizeof(RakNet::MessageID));

	// compare player's version
	char version[100];
	bsIn.Read(version);
	if (strcmp(version, LHMP_VERSION_TEST_HASH) != 0)
	{
		RakNet::BitStream errStr;
		errStr.Write((RakNet::MessageID)ID_GAME_BAD_VERSION);
		peer->Send(&errStr, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
		peer->CloseConnection(packet->systemAddress, true);

		printf("Bad connection attempt. Version hash dismatch. \n");
		return;
	}
	// get network slot for him
	int ID = GetFirstFreeSlot();
	slot[ID].isUsed = true;
	slot[ID].sa = packet->systemAddress;
	// he has a good version
	printf("A connection is incoming. ID: %d \n", ID);
	// he has the right version, so let's send him files needed for game / scripts
	g_CCore->GetFileTransfer()->SendFiles(packet->systemAddress);
}

void	CNetworkManager::OnPlayerFileTransferFinished(RakNet::SystemAddress)
{
	int ID = GetIDFromSystemAddress(packet->systemAddress);
	_Server serInfo;
	serInfo.max_players = this->m_pServerMaxPlayers;
	serInfo.playerid = GetIDFromSystemAddress(packet->systemAddress);
	sprintf(serInfo.server_name, "%s", this->m_pSvrName.c_str());
	serInfo.server_port = this->m_pServerPort;
	
	RakNet::BitStream bsOutR;
	bsOutR.Write((RakNet::MessageID)ID_CONNECTION_FINISHED);
	bsOutR.Write(serInfo); // tu su data, struct
	bsOutR.Write(g_CCore->GetDefaultMap());
	peer->Send(&bsOutR, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
}


void	CNetworkManager::OnPlayerDisconnect(RakNet::Packet* packet)
{
	int ID = GetIDFromSystemAddress(packet->systemAddress);
	// if regular RakNet client lost connection
	if (ID != -1)
	{
		// if it was connected player
		CPlayer* player = g_CCore->GetPlayerPool()->Return(ID);
		if (player != NULL)
		{
			g_CCore->GetScripts()->onPlayerDisconnect(ID);

			if (player->InCar != -1)
			{
				CVehicle* veh = g_CCore->GetVehiclePool()->Return(player->InCar);
				veh->PlayerDisconnect(ID);
			}
			char buff[255];
			if (packet->data[0] == ID_CONNECTION_LOST)
				sprintf(buff, "Player %s lost connection", player->GetNickname());
			else
				sprintf(buff, "Player %s disconnected", player->GetNickname());
			SendMessageToAll(buff);

			if (packet->data[0] == ID_CONNECTION_LOST)
				printf("%s[%i] has lost his connection. \n", player->GetNickname(), ID);
			else
				printf("%s[%i] has disconnected.\n", player->GetNickname(), ID);

			g_CCore->GetPlayerPool()->Delete(ID);

			RakNet::BitStream bsOut;
			bsOut.Write((RakNet::MessageID)ID_GAME_LHMP_PACKET);
			bsOut.Write((RakNet::MessageID)LHMP_PLAYER_DISCONNECT);
			bsOut.Write(ID);
			peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
		}
		slot[ID].isUsed = NULL;
	}
}

void CNetworkManager::Pulse()
{
	for (packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
		{
			RakNet::TimeMS TS = NULL;
			int offset = 0;
			if (packet->data[0] == (MessageID)ID_TIMESTAMP)
			{
				// this packet has a timesteam before raw data
				RakNet::BitStream timestamp(packet->data + 1, sizeof(RakNet::TimeMS) + 1, false);
				timestamp.Read(TS);
				offset = 1 + sizeof(RakNet::TimeMS);

			}
			switch (packet->data[offset])
				{
				case ID_NEW_INCOMING_CONNECTION:
					{
						printf("Incoming connection \n");
					}
					break;
				case ID_DISCONNECTION_NOTIFICATION:
				case ID_CONNECTION_LOST:
				{
									this->OnPlayerDisconnect(packet);
				}
					break;
				// this message is sent by client after server accepts his connection attempt
				case ID_INITLHMP:
				{
									// check client's game version hash
									// if doesn't match, cause disconnect,
									// if does, start file transfer
									  this->OnPlayerConnection(packet);
				}
					break;
				// when whole connection is done, just send him all players / send his to all players
				case ID_CONNECTION_FINISHED:
				{
					int ID = GetIDFromSystemAddress(packet->systemAddress);
					g_CCore->GetPlayerPool()->New(ID);

					//TODO - process info from player

					RakNet::RakString rs;
					float FOV;
					BitStream bsIn(packet->data + offset, packet->length - offset, false);
					bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
					bsIn.Read(rs);
					bsIn.Read(FOV);

					//------------------------------------------------------

					CPlayer* player = g_CCore->GetPlayerPool()->Return(ID);
					player->SetNickname(rs.C_String());
					player->SetIsActive(1);
					player->SetIsSpawned(1);
					player->SetSkin(0);
					player->SetFOV(FOV);

					RakNet::BitStream bsOut;
					bsOut.Write((RakNet::MessageID)ID_GAME_LHMP_PACKET);
					bsOut.Write((RakNet::MessageID)LHMP_PLAYER_CONNECT);
					bsOut.Write(ID);
					bsOut.Write(player->GetNickname());
					bsOut.Write(player->GetSkin());
					peer->Send(&bsOut,IMMEDIATE_PRIORITY,RELIABLE_ORDERED,0,packet->systemAddress,true);
					
					char buff[255];
					sprintf(buff, "Player %s[%i]'s connected server.", player->GetNickname(), ID);
					this->SendMessageToAll(buff);
					printf("Player %s[%i]'s connected server.\n", player->GetNickname(),ID);


					g_CCore->GetScripts()->onPlayerConnect(ID);
					g_CCore->GetScripts()->onPlayerSpawn(ID);

					SendHimOthers(ID);
					SendHimCars(ID);
					SendHimDoors(ID);
					SendHimPickups(ID);
				}
				break;
				case ID_GAME_LHMP_PACKET:
				{
					LHMPPacket(packet,TS);
				}
				break;
				case ID_FILETRANSFER:
				{
					RakNet::BitStream bsIn(packet->data + 1, packet->length - 1, false);
					g_CCore->GetFileTransfer()->HandlePacket(&bsIn,packet->systemAddress);
				}
				break;
				default:
					g_CCore->GetLog()->AddNormalLog("Message with identifier %i has arrived.", packet->data[0]);
					break;
				}
		}
	CNetworkManager::SendSYNC();
	CNetworkManager::SendCarSYNC();
}

void CNetworkManager::LHMPPacket(Packet* packet, RakNet::TimeMS timestamp)
{
	int offset = 1;
	if (timestamp != NULL)
		offset += sizeof(RakNet::MessageID)+sizeof(RakNet::TimeMS);
	switch (packet->data[offset])
	{
		case LHMP_PLAYER_CHAT_MESSAGE:
		{
			int ID = GetIDFromSystemAddress(packet->systemAddress);
			if(ID == -1) return;
			CPlayer* player = g_CCore->GetPlayerPool()->Return(ID);
			if (player != NULL)
			{
				RakString rs;
				RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
				char buff[255];
				char buff2[255];
				bsIn.Read(buff);

				sprintf(buff2, "[%s] %s", player->GetNickname(), buff);
				if (g_CCore->GetScripts()->onPlayerText(ID, buff) == true)
				{
					BitStream bsOut;
					bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
					bsOut.Write((MessageID)LHMP_PLAYER_CHAT_MESSAGE);
					bsOut.Write(buff2);
					printf("[PlayerMessage] ID: %d %s \n", ID, buff);

					for (int i = 0; i < m_pServerMaxPlayers; i++)
					{
						if (slot[i].isUsed == true)
							peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, slot[i].sa, false);
					}
				}
			}
		}
		break;
		case LHMP_PLAYER_COMMAND:
		{
			int ID = GetIDFromSystemAddress(packet->systemAddress);
			if (ID == -1) return;
			RakString rs;
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			char buff[255];
			char params[255];
			bsIn.Read(buff);
			bsIn.Read(params);
			g_CCore->GetScripts()->onPlayerCommand(ID, buff,params);
		}
			break;
		case LHMP_PLAYER_SENT_SYNC_ON_FOOT:
		{
			int ID = GetIDFromSystemAddress(packet->systemAddress);
			if(ID == -1) return;
			CPlayer* player = g_CCore->GetPlayerPool()->Return(ID);
			if (player != NULL)
			{
				RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
				SYNC::ON_FOOT_SYNC	syncData;
				bsIn.Read(syncData);
				player->OnSync(syncData,timestamp);
			}
		}
		break;
		case LHMP_PLAYER_SENT_CAR_UPDATE:
		{
			int ID = GetIDFromSystemAddress(packet->systemAddress);
			if (ID == -1) return;
			CPlayer* player = g_CCore->GetPlayerPool()->Return(ID);
			if (player != NULL)
			{
				RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
				SYNC::IN_CAR	syncData;
				bsIn.Read(syncData);
				player->OnCarUpdate(syncData);
			}
		}
			break;
		case LHMP_PLAYER_CHANGESKIN:
		{
			int ID = GetIDFromSystemAddress(packet->systemAddress);
			if(ID == -1) return;
			CPlayer* player = g_CCore->GetPlayerPool()->Return(ID);
			if (player != NULL)
			{
				int pID,skin;
				RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
				bsIn.Read(pID);
				bsIn.Read(skin);
				player->OnChangeSkin(skin);
			}
		}
		break;
		case LHMP_PLAYER_RESPAWN:
		{
			//RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);

			int ID = GetIDFromSystemAddress(packet->systemAddress);
			if(ID == -1) return;
			CPlayer* player = g_CCore->GetPlayerPool()->Return(ID);
			if (player != NULL)
			{
				player->OnRespawn();
			}
		}
		break;
		case LHMP_PLAYER_ADDWEAPON:
		{
			int wepID, wepLoaded,wepHidden;
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			//peer->Send(&bsIn,IMMEDIATE_PRIORITY,RELIABLE_ORDERED,0,UNASSIGNED_SYSTEM_ADDRESS,true);

			bsIn.IgnoreBytes(sizeof(int));
			bsIn.Read(wepID);
			bsIn.Read(wepLoaded);
			bsIn.Read(wepHidden);
			int ID = GetIDFromSystemAddress(packet->systemAddress);
			if(ID == -1) return;
			CPlayer* player = g_CCore->GetPlayerPool()->Return(ID);
			if (player != NULL)
			{
				player->OnAddWeapon(wepID, wepLoaded, wepHidden);
			}
		}
		break;
		case LHMP_PLAYER_DELETEWEAPON:
		{
			int wepID;
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			bsIn.IgnoreBytes(sizeof(int));
			bsIn.Read(wepID);
			int ID = GetIDFromSystemAddress(packet->systemAddress);
			if(ID == -1) return;
			CPlayer* player = g_CCore->GetPlayerPool()->Return(ID);
			if (player != NULL)
			{
				player->OnDeleteWeapon(wepID);
			}
		}
		break;
		case LHMP_PLAYER_SWITCHWEAPON:
		{
			int wepID;
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);

			bsIn.IgnoreBytes(sizeof(int));
			bsIn.Read(wepID);
			int ID = GetIDFromSystemAddress(packet->systemAddress);
			if(ID == -1) return;
			CPlayer* player = g_CCore->GetPlayerPool()->Return(ID);
			if (player != NULL)
			{
				player->OnSwitchWeapon(wepID);
			}
		}
		break;
		case LHMP_PLAYER_SHOOT:
		{
			float x, y, z;
			int ID;
			int weapon;
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			bsIn.Read(ID);
			bsIn.Read(x);
			bsIn.Read(y);
			bsIn.Read(z);
			bsIn.Read(weapon);

			ID = GetIDFromSystemAddress(packet->systemAddress);
			if(ID == -1) return;
			CPlayer* player = g_CCore->GetPlayerPool()->Return(ID);
			if (player != NULL)
			{
				player->SetCurrentWeapon(weapon);
				player->OnPlayerShoot(x, y, z);
			}

		}
		break;
		case LHMP_PLAYER_THROWGRANADE:
		{
			Vector3D position;
			int ID = GetIDFromSystemAddress(packet->systemAddress);
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			bsIn.Read(position);

			CPlayer* player = g_CCore->GetPlayerPool()->Return(ID);
			if (player != NULL)
			{
				player->OnPlayerThrowGranade(position);
			}

		}
			break;
		case LHMP_PLAYER_DEATH:
		{
			int ID = GetIDFromSystemAddress(packet->systemAddress), killerID,reason,part;
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);

			bsIn.Read(killerID); // -1 if undetected / suicide
			bsIn.Read(reason); 
			bsIn.Read(part);

			if (killerID == -1)
				killerID = ID;
			g_CCore->GetScripts()->onPlayerIsKilled(ID, killerID,reason,part);
			BitStream bsOut;
			bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
			bsOut.Write((MessageID)LHMP_PLAYER_DEATH);
			bsOut.Write(ID);
			bsOut.Write(reason);
			bsOut.Write(part);
			peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);

			CPlayer* player = g_CCore->GetPlayerPool()->Return(ID);
			
			//if player exists
			if (player)
			{
				// if he is in a car
				if (player->InCar != -1)
				{
					// get him out of car (at least at server side)
					CVehicle* veh = g_CCore->GetVehiclePool()->Return(player->InCar);
					if (veh)
					{
						veh->PlayerExit(ID);
					}
				}
			}
		}
		break;
		case LHMP_PLAYER_DEATH_END:
		{
			int ID = GetIDFromSystemAddress(packet->systemAddress);
			//killerID, reason, part;
								 
			BitStream bsOut;
			bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
			bsOut.Write((MessageID)LHMP_PLAYER_DEATH_END);
			bsOut.Write(ID);
			peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);

		}
			break;
		case LHMP_PLAYER_HIT:
		{
			int ID = GetIDFromSystemAddress(packet->systemAddress), attackerID;
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			bsIn.Read(attackerID);
			g_CCore->GetScripts()->onPlayerHit(ID, attackerID);
		}
			break;
		// Vehicles
		case LHMP_VEHICLE_CREATE:
		{
			//std::cout << "baf" << std::endl;
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			VEH::CREATE vehicle_data;
			bsIn.Read(vehicle_data);
			int ID = g_CCore->GetVehiclePool()->New();
			CVehicle* veh = g_CCore->GetVehiclePool()->Return(ID);
			veh->SetPosition(vehicle_data.position);
			veh->SetRotation(vehicle_data.rotation);
			veh->SetSkin(vehicle_data.skinID);
			veh->ToggleRoof(vehicle_data.roofState);
			veh->SetSirenState(vehicle_data.siren);

			vehicle_data.damage = veh->GetDamage();
			vehicle_data.shotdamage = veh->GetShotDamage();
			vehicle_data.roofState = veh->GetRoofState();
			vehicle_data.engineState = veh->GetEngineState();

			vehicle_data.siren = veh->GetSirenState();
			vehicle_data.ID = ID;
			for (int i = 0; i < 4;i++)
				vehicle_data.seat[i] = -1;
			BitStream bsOut;
			bsOut.Write((RakNet::MessageID)ID_GAME_LHMP_PACKET);
			bsOut.Write((RakNet::MessageID)LHMP_VEHICLE_CREATE);
			bsOut.Write(vehicle_data);
			peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0,UNASSIGNED_RAKNET_GUID, true);
		}
		break;
		case LHMP_VEHICLE_SYNC:
		{
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			VEH::SYNC syncData;
			bsIn.Read(syncData);
			CVehicle* veh = g_CCore->GetVehiclePool()->Return(syncData.ID);
			if (veh != NULL)
			{
				veh->SetPosition(syncData.position);
				veh->SetRotation(syncData.rotation);
				veh->SetSpeed(syncData.speed);
				veh->SetWheelsAngle(syncData.wheels);
				veh->SetHornState(syncData.horn);
				veh->SetSecondRot(syncData.secondRot);
				veh->SetOnGas(syncData.gasOn);
				veh->SetTimeStamp(timestamp);
				veh->SetFuel(syncData.fuel);

				for (int i = 0; i < 4; i++)
				{
					if (veh->GetSeat(i) != -1)
					{
						CPlayer* play = g_CCore->GetPlayerPool()->Return(veh->GetSeat(i));
						if (play != NULL)
						{
							play->SetPosition(syncData.position);
							play->SetTimeStamp(timestamp);
						}
					}
				}
			}
		}
		break;
		case LHMP_PLAYER_ENTERED_VEHICLE:
		{
			int vehID, seatID;
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			bsIn.Read(vehID);
			bsIn.Read(seatID);
			g_CCore->GetVehiclePool()->Return(vehID)->PlayerEnter(GetIDFromSystemAddress(packet->systemAddress),seatID);
			BitStream bsOut;
			bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
			bsOut.Write((MessageID)LHMP_PLAYER_ENTERED_VEHICLE);
			bsOut.Write(GetIDFromSystemAddress(packet->systemAddress));
			bsOut.Write(vehID);
			bsOut.Write(seatID);
			peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
			//std::cout << "EnterVeh" << GetIDFromSystemAddress(packet->systemAddress) << " - " << vehID << std::endl;

			g_CCore->GetScripts()->onPlayerEnterVehicle(GetIDFromSystemAddress(packet->systemAddress), vehID, seatID);
		}
		break;
		case LHMP_PLAYER_EXIT_VEHICLE:
		{
			int vehID;
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			bsIn.Read(vehID);
			g_CCore->GetVehiclePool()->Return(vehID)->PlayerExit(GetIDFromSystemAddress(packet->systemAddress));
			BitStream bsOut;
			bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
			bsOut.Write((MessageID)LHMP_PLAYER_EXIT_VEHICLE);
			bsOut.Write(GetIDFromSystemAddress(packet->systemAddress));
			bsOut.Write(vehID);
			peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);

			g_CCore->GetScripts()->onPlayerExitVehicle(GetIDFromSystemAddress(packet->systemAddress), vehID);
			//std::cout << "ExitVeh" << GetIDFromSystemAddress(packet->systemAddress) << " - " << vehID << std::endl;
		}
		break;
		case LHMP_PLAYER_EXIT_VEHICLE_FINISH:
		{
			BitStream bsOut;
			bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
			bsOut.Write((MessageID)LHMP_PLAYER_EXIT_VEHICLE_FINISH);
			bsOut.Write(GetIDFromSystemAddress(packet->systemAddress));
			peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);

			g_CCore->GetScripts()->onPlayerExitVehicleFinish(GetIDFromSystemAddress(packet->systemAddress));
			//std::cout << "ExitVehFinish" << GetIDFromSystemAddress(packet->systemAddress) << std::endl;
		}
			break;
		case LHMP_VEHICLE_JACK:
		{
			int vehID,seatID;
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			bsIn.Read(vehID);
			bsIn.Read(seatID);
			CVehicle* veh = g_CCore->GetVehiclePool()->Return(vehID);
			if (veh->GetSeat(seatID) == -1)				// bad case
				break;
			veh->PlayerExit(veh->GetSeat(seatID));
			//g_CCore->GetVehiclePool()->Return(vehID)->PlayerExit(GetIDFromSystemAddress(packet->systemAddress));
			BitStream bsOut;
			bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
			bsOut.Write((MessageID)LHMP_VEHICLE_JACK);
			bsOut.Write(GetIDFromSystemAddress(packet->systemAddress));
			bsOut.Write(vehID);
			bsOut.Write(seatID);
			peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
			//std::cout << "JackVeh" << GetIDFromSystemAddress(packet->systemAddress) << " - " << vehID << seatID <<std::endl;
		}
		break;

		case LHMP_VEHICLE_TOGGLE_ENGINE:
		{
			int vehID;
			BYTE state;

			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			bsIn.Read(vehID);
			bsIn.Read(state);

			//serverside set engine status

			CVehicle* veh = g_CCore->GetVehiclePool()->Return(vehID);
			if (veh != NULL)
			{
				veh->ToggleEngine(state);
			}

			BitStream bsOut;
			bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
			bsOut.Write((MessageID)LHMP_VEHICLE_TOGGLE_ENGINE);
			bsOut.Write(vehID);
			bsOut.Write(state);
			peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
		}
		break;

		case LHMP_VEHICLE_DELETE:
		{
			int vehID;
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			bsIn.Read(vehID);
			g_CCore->GetVehiclePool()->Delete(vehID);
			//CVehicle* veh = g_CCore->GetVehiclePool()->Return(vehID);
			//delete veh;
			/*veh->~CVehicle();
			veh = NULL;
			*/
			BitStream bsOut;
			bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
			bsOut.Write((MessageID)LHMP_VEHICLE_DELETE);
			bsOut.Write(vehID);
			peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
		}
			break;
		case LHMP_VEHICLE_DAMAGE:
		{
			int vehID;
			float damage;
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			bsIn.Read(vehID);
			bsIn.Read(damage);
			CVehicle* veh = g_CCore->GetVehiclePool()->Return(vehID);
			if (veh != NULL)
			{
				veh->SetDamage(damage);
				BitStream bsOut;
				bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
				bsOut.Write((MessageID)LHMP_VEHICLE_DAMAGE);
				bsOut.Write(vehID);
				bsOut.Write(damage);
				peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
			}
		}
			break;
		case LHMP_VEHICLE_SHOTDAMAGE:
		{
			int vehID;
			byte damage;
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			bsIn.Read(vehID);
			bsIn.Read(damage);
			CVehicle* veh = g_CCore->GetVehiclePool()->Return(vehID);
			if (veh != NULL)
			{
				veh->SetShotDamage(damage);
				BitStream bsOut;
				bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
				bsOut.Write((MessageID)LHMP_VEHICLE_SHOTDAMAGE);
				bsOut.Write(vehID);
				bsOut.Write(damage);
				peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
			}
		}
			break;
		case LHMP_VEHICLE_ON_EXPLODED:
		{
			int vehID;
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			bsIn.Read(vehID);
			CVehicle* veh = g_CCore->GetVehiclePool()->Return(vehID);
			if (veh != NULL)
			{
				if (veh->IsExploded() == false)
				{
					veh->SetExplodeTime(g_CCore->GetTickManager()->GetTime());
					veh->SetExploded(true);

					BitStream bsOut;
					bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
					bsOut.Write((MessageID)LHMP_VEHICLE_ON_EXPLODED);
					bsOut.Write(vehID);
					peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
				}
			}
		}
		break;
		case LHMP_DOOR_SET_STATE:
		{
			int state;
			bool facing;
			char name[200];
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			bsIn.Read(name);
			bsIn.Read(state);
			bsIn.Read(facing);
			g_CCore->GetDoorPool()->Push(name,(bool)state,facing);
			BitStream bsOut;

			bool stateOut = state;
			bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
			bsOut.Write((MessageID)LHMP_DOOR_SET_STATE);
			bsOut.Write(name);
			bsOut.Write(stateOut);
			bsOut.Write(facing);
			peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
		}
			break;
		case LHMP_SCRIPT_ON_KEY_PRESSED:
		{
			int key;
			RakNet::BitStream bsIn(packet->data + offset + 1, packet->length - offset - 1, false);
			bsIn.Read(key);
			g_CCore->GetScripts()->onPlayerKeyPressed(this->GetIDFromSystemAddress(packet->systemAddress),key);
		}
			break;

	}
}

void CNetworkManager::SendSYNC()
{
	for(int i=0; i < m_pServerMaxPlayers; i++)
	{
		CPlayer* player = g_CCore->GetPlayerPool()->Return(i);
		if(player == NULL) continue;
		if (player->InCar == -1)
		{
			if (player->ShouldUpdate())
			{
				//strPlayer* actual = &g_CCore->GetPlayers()[i];
				//if(actual->isSpawned)
				if(player->IsSpawned())
				{
					SYNC::ON_FOOT_SYNC syncData;
					/*syncData.ID					= i;
					syncData.position			= actual->position;
					syncData.degree				= actual->degree;
					syncData.degree_second		= actual->degree_second;
					syncData.degree_third		= actual->degree_third;
					syncData.status				= actual->status;
					syncData.health				= actual->health;
					syncData.isDucking			= actual->isDucking;
					syncData.isAim				= actual->isAim;*/
					syncData.ID					= i;
					syncData.position			= player->GetPosition();
					Vector3D rot				= player->GetRotation();
					syncData.degree				= rot.x;
					syncData.degree_second		= rot.z;
					//syncData.degree_third		= actual->degree_third;
					syncData.status				= player->GetStatus();
					syncData.health				= player->GetHealth();
					syncData.isDucking			= player->IsDucking();
					syncData.isAim				= player->IsAim();
					syncData.isCarAnim			= player->IsCarAnim();

					BitStream bsOut;
					bsOut.Write((MessageID)ID_TIMESTAMP);
					bsOut.Write(player->GetTimeStamp());
					bsOut.Write((MessageID) ID_GAME_LHMP_PACKET);
					bsOut.Write((MessageID) LHMP_PLAYER_SENT_SYNC_ON_FOOT);
					bsOut.Write(syncData);
					peer->Send(&bsOut,LOW_PRIORITY,UNRELIABLE,0,slot[i].sa,true);
					player->SetShouldUpdate(false);	// wait till next update
				}
			}
		}
		else
		{
			SYNC::IN_CAR syncData;
			syncData.aim		= player->GetCarAim();
			syncData.health		= player->GetHealth();
			syncData.ID			= i;
			syncData.isAiming	= player->IsAim();
			BitStream bsOut;
			bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
			bsOut.Write((MessageID)LHMP_PLAYER_SENT_CAR_UPDATE);
			bsOut.Write(syncData);
			peer->Send(&bsOut, LOW_PRIORITY, UNRELIABLE, 0, slot[i].sa, true);
			// in car sync

		}
	}
}
void CNetworkManager::SendCarSYNC()
{
	for (int i = 0; i < MAX_VEHICLES; i++)
	{
		CVehicle* veh = g_CCore->GetVehiclePool()->Return(i);
		if (veh != NULL)
		{
			//if ()
			if (veh->GetSeat(0) == -1 || veh->ShouldUpdate())
			{
				veh->SendSync();
				veh->SetShouldUpdate(false);	// wait till next update
			}
		}
	}
}
void CNetworkManager::SendMessageToAll(char * message)
{
	BitStream bsOut;
	bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
	bsOut.Write((MessageID)LHMP_PLAYER_CHAT_MESSAGE);
	bsOut.Write(message);
	peer->Send(&bsOut,IMMEDIATE_PRIORITY,RELIABLE_ORDERED,0,RakNet::UNASSIGNED_RAKNET_GUID,true);
	/*		for(int i = 0; i < m_pServerMaxPlayers;i++)
			{
				if(slot[i].isUsed == true)
					peer->Send(&bsOut,IMMEDIATE_PRIORITY,RELIABLE_ORDERED,0,slot[i].sa,false);
			}
			*/
}

void CNetworkManager::SendHimOthers(int he)
{
	int count = 0;
	SystemAddress sa = this->GetSystemAddressFromID(he);

	for(int i=0; i < m_pServerMaxPlayers; i++)
	{
		if(i == he) continue;
		CPlayer* player = g_CCore->GetPlayerPool()->Return(i);
		if(player == NULL) continue;
		//strPlayer* actual = &g_CCore->GetPlayers()[i];
		//if(actual->isSpawned)
		if(player->IsSpawned())
		{
			BitStream bsOut;
			bsOut.Write((MessageID) ID_GAME_LHMP_PACKET);
			bsOut.Write((MessageID) LHMP_PLAYER_CONNECT);
			bsOut.Write(i);
			bsOut.Write(player->GetNickname());
			bsOut.Write(player->GetSkin());
			//bsOut.Write(actual->nickname);
			//bsOut.Write(actual->skinID);
			peer->Send(&bsOut,IMMEDIATE_PRIORITY,RELIABLE_ORDERED,0,sa,false);

			//Send weapon info
			for(int i = 0; i < 8;i++)
			{
				SWeapon* wep = player->GetWeapon(i);
				if(wep->wepID == NULL) continue;
				bsOut.Reset();
				bsOut.Write((MessageID) ID_GAME_LHMP_PACKET);
				bsOut.Write((MessageID) LHMP_PLAYER_ADDWEAPON);
				bsOut.Write(i);
				bsOut.Write(wep->wepID);
				bsOut.Write(wep->wepLoaded);
				bsOut.Write(wep->wepHidden);
				peer->Send(&bsOut,IMMEDIATE_PRIORITY,RELIABLE_ORDERED,0,sa,false);
			}
			count++;
		}
	}
	//std::cout << "SendHimOthers Count - " << count << std::endl;
}

void CNetworkManager::SendHimDoors(int he)
{
	SystemAddress sa = this->GetSystemAddressFromID(he);
	sDoor* door = g_CCore->GetDoorPool()->GetStart();
	while (door != NULL)
	{
		BitStream bsOut;
		bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
		bsOut.Write((MessageID)LHMP_DOOR_SET_STATE);
		bsOut.Write(door->name);
		bsOut.Write(door->state);
		bsOut.Write(door->facing);
		peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, sa, false);
		door = door->nextDoor;
	}
}

void CNetworkManager::SendPingUpdate()
{
	for(int i=0; i < m_pServerMaxPlayers; i++)
	{
		CPlayer* player = g_CCore->GetPlayerPool()->Return(i);
		if (player != NULL)
		{
			if (player->IsSpawned())
			{
				SystemAddress sa = GetSystemAddressFromID(i);
				BitStream bsOut;
				bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
				bsOut.Write((MessageID)LHMP_PLAYER_PING);
				bsOut.Write(i);
				bsOut.Write(peer->GetLastPing(sa));
				peer->Send(&bsOut, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_RAKNET_GUID, true);
				peer->Ping(sa);
			}
		}
	}
}

int	CNetworkManager::GetPort()
{
	return this->m_pServerPort;
}

void CNetworkManager::SendHimCars(int ID)
{
	CPlayer* player = g_CCore->GetPlayerPool()->Return(ID);
	if (player != NULL)
	{
		for (int i = 0; i < MAX_VEHICLES; i++)
		{
			CVehicle* veh = g_CCore->GetVehiclePool()->Return(i);
			if (veh != NULL)
			{
				VEH::CREATE vehicle;
				vehicle.ID = i;
				vehicle.isSpawned	= veh->IsSpawned();
				vehicle.position	= veh->GetPosition();
				vehicle.rotation	= veh->GetRotation();
				vehicle.skinID		= veh->GetSkin();
				vehicle.damage		= veh->GetDamage();
				vehicle.shotdamage	= veh->GetShotDamage();
				vehicle.roofState	= veh->GetRoofState();
				vehicle.engineState = veh->GetEngineState();
				//std::cout << "Damage: " << vehicle.damage << std::endl;
				for (int i = 0; i < 4; i++)
					vehicle.seat[i] = veh->GetSeat(i);
				BitStream bsOut;
				bsOut.Write((MessageID)ID_GAME_LHMP_PACKET);
				bsOut.Write((MessageID)LHMP_VEHICLE_CREATE);
				bsOut.Write(vehicle);
				peer->Send(&bsOut, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, GetSystemAddressFromID(ID), false);
			}
		}
	}
}

RakNet::RakPeerInterface*	CNetworkManager::GetPeer()
{
	return this->peer;
}

void	CNetworkManager::SendHimPickups(int ID)
{
	for (int i = 0; i < MAX_PICKUPS; i++)
	{
		CPickup* pickup = g_CCore->GetPickupPool()->Return(i);
		if (pickup)
		{
			pickup->SendIt(ID);
		}
	}
}
