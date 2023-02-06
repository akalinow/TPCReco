/**
 * @file CentralConfig.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of CentralConfig class
 */

#include <CentralConfig.hh>


CentralConfig* CentralConfig::instance=nullptr;

CentralConfig* CentralConfig::GetInstance(std::string configFileName)
{
	if(instance==nullptr)
		instance=new CentralConfig(configFileName);
	return instance;
}

CentralConfig::CentralConfig(std::string configFileName)
{
	if(configFileName=="")
		configFileName="config.xml";

	result = config.load_file(configFileName.c_str());
	
	if (!result)
		std::cerr<<"Error opening central config file: "<<configFileName<<". Check if file exists, and that the formatting is correct!!"<<std::endl;
}

bool CentralConfig::Has(std::string field_name)
{
	return !GetChild(config,field_name.c_str()).empty();
}

pugi::xml_node CentralConfig::GetChild(pugi::xml_node parent, std::string field_name)
{
	return parent.child(field_name.c_str());
}

