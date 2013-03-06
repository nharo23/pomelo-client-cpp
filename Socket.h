#pragma once

#ifdef WIN32
#include <WinSock2.h>
#else
#include <socket>
#endif

#include <set>
#include <string>
#include <iostream>

namespace PomeloCpp
{
	namespace Private
	{
		class SocketHandler
		{
		public:
			virtual ~SocketHandler() {}

			virtual void onReceiveData( const void* pData, size_t size ) {}
			virtual void onConnected() {}
			virtual void onDisconnected() {}	
		};

		class Socket
		{
			enum ClientState
			{
				eClientState_None,
				eClientState_Connecting,
				eClientState_Connected,
			};
		public:
			Socket()
				: m_socketHandle(INVALID_SOCKET)
				, m_host("127.0.0.1")
				, m_port(9999)
				, m_state( eClientState_None )
			{}

			Socket( const char* host, unsigned short port )
			{
				connect( host, port );
			}

			~Socket()
			{
				disconnect();
			}

			bool connect( const char* host, unsigned short port )
			{
#ifdef WIN32
				WSADATA wsaData;
				WORD sockVersion = MAKEWORD(2, 0);
				WSAStartup( sockVersion, &wsaData );
#endif

				m_host = host;
				m_port = port;

				int af = AF_INET;
				int sockProtocal = IPPROTO_TCP;
				int type = SOCK_STREAM;

				m_socketHandle = socket(af, type, sockProtocal);
				if ( m_socketHandle == INVALID_SOCKET )
				{
					disconnect();
					return false;
				}

				m_remoteAddress.sin_addr.s_addr = inet_addr( host );
				m_remoteAddress.sin_port = ::htons(port);
				m_remoteAddress.sin_family = AF_INET;

				int result = ::connect( m_socketHandle, (const sockaddr FAR*)&m_remoteAddress, sizeof( m_remoteAddress ) );
				if ( result == SOCKET_ERROR)
				{
					disconnect();
					return false;
				}

				m_state = eClientState_Connecting;
				return true;
			}

			void disconnect()
			{		
				for ( std::set<SocketHandler*>::iterator i = m_handlers.begin(); i != m_handlers.end(); ++i )
					(*i)->onDisconnected();

				closesocket( m_socketHandle );
				m_socketHandle = INVALID_SOCKET;
				m_state = eClientState_None;
				
#ifdef WIN32
				WSACleanup();
#endif
			}

			void addHandler( SocketHandler* pHandler )
			{
				if ( pHandler )
					m_handlers.insert( pHandler );
			}

			void removeHandler( SocketHandler* pHandler )
			{
				if ( pHandler )
					m_handlers.erase( pHandler );
			}

			bool isConnected() const 
			{ 
				return m_socketHandle != INVALID_SOCKET && m_state == eClientState_Connected;
			}

			void updateStatus()
			{
				fd_set readSet;
				fd_set writeSet;

				timeval timeVal;
				timeVal.tv_sec = 0;
				timeVal.tv_usec = 0;

				if ( m_state == eClientState_None )
					return;
				else if ( m_state == eClientState_Connecting )
				{
					writeSet.fd_count = 1;
					writeSet.fd_array[0] = m_socketHandle;

					int result = select( 0, NULL, &writeSet, NULL, &timeVal );
					if ( result > 0 )
					{                
						if ( m_socketHandle != INVALID_SOCKET )
						{
							m_state = eClientState_Connected;

							for ( std::set<SocketHandler*>::iterator i = m_handlers.begin(); i != m_handlers.end(); ++i )
								(*i)->onConnected();
						}
					}

					return;
				}

				if ( m_socketHandle == INVALID_SOCKET )
				{
					if (m_state == eClientState_Connected )				 
					{
						disconnect();
					}

					return;
				}

				readSet.fd_count = 1;
				readSet.fd_array[0] = m_socketHandle;

				int result = select( 0, &readSet, NULL, NULL, &timeVal );

				// 这样就可以保证recv到东西？
				if ( result > 0 && readSet.fd_count > 0 )
				{
					const int MAX_BUFF_LEN = 1024;
					static char buffer[MAX_BUFF_LEN] = {0};  

					int recvLen = recv( m_socketHandle, buffer, MAX_BUFF_LEN, 0 );			 
					if ( recvLen == 0 )
					{
						// connection closed
						std::cout << "remote connection closed gracefully" << std::endl;
						disconnect();
						return;
					}
					else if ( recvLen == SOCKET_ERROR )
					{				
						std::cout << "SOCKET_ERROR when receive data" << std::endl;
						disconnect();
						return;
					}

					if (recvLen > 0)
					{
						onReceiveData( buffer, recvLen );
					}
				}
			}

			int sendData( const void* pData, size_t size )
			{
				return send( m_socketHandle, (const char*)pData, size, 0 );
			}

			void onReceiveData( const void* pData, size_t size )
			{		
				for ( std::set<SocketHandler*>::iterator i = m_handlers.begin(); i != m_handlers.end(); ++i )
					(*i)->onReceiveData( pData, size );
			}
		private:
			int m_socketHandle;

			std::string m_host;
			unsigned short m_port;
			sockaddr_in m_remoteAddress;

			std::set<SocketHandler*> m_handlers;
			ClientState m_state;
		};
	}
}