#pragma once

#include <json/json.h>
#include "Socket.h"

namespace PomeloCpp
{	
	typedef void* MsgHandler;

	class ProtoBufToJSONConvertor
	{

	};

	class PomeloClient
		: public Private::SocketHandler
	{
	public:
		PomeloClient(void);		
		PomeloClient( const char* host, unsigned short port );
		virtual ~PomeloClient(void);

		bool init( const char* host, unsigned short port );	
		void updateStatus();
		bool connected() const;
		void disconnect();
		bool connectionValid() const { return m_connection != NULL; }

		void request( const char* route, Json::Value* msg = NULL, MsgHandler callback = NULL );
		void notify( const char* route, Json::Value* msg = NULL );

		void registerMsgHandler( const char* route, MsgHandler pHandler );

	protected:
		void sendMesage( const char* msg, size_t len );
		void receiveMessage( const char* msg, size_t len );

		virtual void onReceiveData( const void* pData, size_t size );
		virtual void onConnected();
		virtual void onDisconnected();

	private:
		int m_requestId;
		Private::Socket* m_connection;

		typedef std::vector<MsgHandler> MsgHandlerList;
		typedef std::map<int, MsgHandlerList > ResponceMsgHandlerMap;
		typedef std::map<std::string, MsgHandlerList > BroadcastMsgHandlerMap;

		ResponceMsgHandlerMap m_responseHandlers;
		BroadcastMsgHandlerMap m_broadcastHandlers;
	};
}


