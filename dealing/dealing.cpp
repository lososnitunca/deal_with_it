#include "stdafx.h"

class CManager
{
private:
	CManagerFactory   m_factory;
	CManagerInterface *m_manager;
public:
	CManager() : m_factory("mtmanapi.dll"), m_manager(NULL)
	{
		m_factory.WinsockStartup();
		if (m_factory.IsValid() == FALSE || (m_manager = m_factory.Create(ManAPIVersion)) == NULL)
		{
			std::cout << "Failed to create MetaTrader 4 Manager API interface" << std::endl;
			return;
		}
	}
	~CManager()
	{
		if (m_manager != NULL)
		{
			if (m_manager->IsConnected())
				m_manager->Disconnect();
			m_manager->Release();
			m_manager = NULL;
		}
		m_factory.WinsockCleanup();
	}
	bool IsValid()
	{
		return(m_manager != NULL);
	}
	CManagerInterface* operator->()
	{
		return(m_manager);
	}
};

CManager manager;

class ParserInit // init class for parsing ini file
{
private:
	boost::property_tree::ptree iniTree;
	std::string m_server;
	int m_login;
	std::string m_password;
	double m_pips;
	std::vector<int> m_Logins{ 0 };
	std::string m_path;

public:
	ParserInit(std::string path = "conf.ini") : m_path(path) // we believe that we have Server, Manager, Timer, Logins sections in file
	{
		try
		{
			boost::property_tree::read_ini(m_path, iniTree);

			boost::property_tree::ptree& iniServer = iniTree.get_child("Server");
			boost::property_tree::ptree& iniManager = iniTree.get_child("Manager");
			boost::property_tree::ptree& iniPips = iniTree.get_child("Pips");
			boost::property_tree::ptree& iniLogins = iniTree.get_child("Logins");

			m_server = iniServer.get<std::string>("adress");
			m_login = iniManager.get<int>("login");
			m_password = iniManager.get<std::string>("password");
			m_pips = iniPips.get<double>("pips");

			int counter = iniLogins.size(); // init list of logins opening orders
			m_Logins.resize(counter);
			counter = 0;
			for (auto&i : iniLogins)
			{
				m_Logins[counter] = i.second.get<int>("");
				++counter;
			}
			std::sort(m_Logins.begin(), m_Logins.end());
		}
		catch (boost::property_tree::ini_parser_error)
		{
			std::cout << "No ini file or bad file structure" << std::endl << std::endl;
		}
	}
	LPCSTR adress() // ip:port function
	{
		LPCSTR server = m_server.c_str();
		return server;
	}
	int login() // manager login function
	{
		return m_login;
	}
	LPCSTR password() // manager password function
	{
		LPCSTR password = m_password.c_str();
		return password;
	}
	double pips() // closing timer function
	{
		return (m_pips/100000);
	}
	std::vector<int> logins() // list of logins opening orders function
	{
		return m_Logins;
	}
};

class MTfunctions // class for project functions
{
public:
	struct m_param // struct for orders functions = parametr "param"
	{
		double ms_pips; // pips from ini file
		std::vector<int> ms_Logins; // list of traders logins whose logins are tracked
	};
private:
	m_param c_param;
public:
	static m_param st_param;
	MTfunctions(m_param param) : c_param(param)
	{
		st_param = c_param;
		//здесь в конструктор будут передаваться инициализированные из инишника данные. 
	}

	static void _stdcall DealingNotify(int code) // pumping callback function, in parametr param we have struct with time and logins list
	{
		switch (code)
		{
		case DEAL_START_DEALING: // message about successfully start dealing
			std::cout << "Dealing`ve started successfully" << std::endl;
		break;
		case DEAL_REQUEST_NEW: // processing dealing requests
			RequestInfo info = { NULL };
			int res = FALSE;
			std::cout << "We have new request! So, let`s do it!" << std::endl;
			if ((res = manager->DealerRequestGet(&info)) == RET_OK)
			{
				res = manager->DealerRequestGet(&info);
				if (std::binary_search(st_param.ms_Logins.begin(), st_param.ms_Logins.end(), info.login))
				{
					if (info.trade.type == TT_ORDER_MK_OPEN || info.trade.type == TT_ORDER_REQ_OPEN || info.trade.type == TT_ORDER_IE_OPEN)
					{
						switch (info.trade.cmd)
						{
						case OP_BUY:
							std::cout << "We have TORADORA1!" << std::endl;
							//info.prices[0] += st_param.ms_pips;
							info.prices[1] = info.prices[0] + st_param.ms_pips;
							break;
						case OP_SELL:
							std::cout << "We have ORA-ORA2!" << std::endl;
							//info.prices[0] -= st_param.ms_pips;
							info.prices[0] -= st_param.ms_pips;
							break;
						/*case OP_BUY_LIMIT:
							std::cout << "We have TORADORA3!" << std::endl;
							info.trade.price = info.trade.tp + st_param.ms_pips;
							break;
						case OP_SELL_LIMIT:
							std::cout << "We have ORA-ORA4!" << std::endl;
							info.trade.price = info.trade.sl - st_param.ms_pips;
							break;
						case OP_BUY_STOP:
							std::cout << "We have TORADORA5!" << std::endl;
							info.trade.price = info.trade.tp + st_param.ms_pips;
							break;
						case OP_SELL_STOP:
							std::cout << "We have ORA-ORA6!" << std::endl;
							info.trade.price = info.trade.sl - st_param.ms_pips;
							break;*/
						}
					}
				}
			}
			res = manager->DealerSend(&info, FALSE, FALSE);
		break;
		}
	}
};

MTfunctions::m_param MTfunctions::st_param = { NULL };

int main()
{
	ParserInit Init("conf.ini"); // initialization ini file configurations
	if (Init.login() == 0)
	{
		system("pause");
		return -1;
	}

	MTfunctions::m_param param;

	param.ms_Logins = Init.logins(); // initialization traders logins from ini file
	param.ms_pips = Init.pips();

	std::cout << param.ms_pips << std::endl;

	MTfunctions InitW(param);
	
	std::cout << "MetaTrader 4 Manager API: Test project" << std::endl;
	std::cout << "Close opened orders by timer for users" << std::endl;
	std::cout << "Copyright ebanye sobaki s osteohondrozom" << std::endl << std::endl;

	int res = RET_ERROR;

	if (((res = manager->Connect(Init.adress())) != RET_OK) || ((res = manager->Login(Init.login(), Init.password())) != RET_OK))
	{
		std::cout << "Connect to " << Init.adress() << " as " << Init.login() << " failed " << manager->ErrorDescription(res) << std::endl;
		system("pause");
		return res;
	}
	else
	{
		if ((res = manager->DealerSwitch(MTfunctions::DealingNotify, 0, 0)) != 0)
		{
			std::cout << "Dealing failed" << std::endl;
			system("pause");
			manager->Disconnect();
			return res;
		}
		else
		{
			std::cout << "Dealer connect to " << Init.adress() << " as " << Init.login() << " successfully: " << res << std::endl;
		}
	}

	system("pause");
	
	manager->Disconnect();

	return 0;
}