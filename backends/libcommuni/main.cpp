/*
 * Copyright (C) 2008-2009 J-P Nurmi jpnurmi@gmail.com
 *
 * This example is free, and not covered by LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 */

#include "transport/config.h"
#include "transport/networkplugin.h"
#include "session.h"
#include <QtCore>
#include <QtNetwork>
#include "Swiften/EventLoop/Qt/QtEventLoop.h"
#include "ircnetworkplugin.h"
#include "singleircnetworkplugin.h"

#include "log4cxx/logger.h"
#include "log4cxx/consoleappender.h"
#include "log4cxx/patternlayout.h"
#include "log4cxx/propertyconfigurator.h"
#include "log4cxx/helpers/properties.h"
#include "log4cxx/helpers/fileinputstream.h"
#include "log4cxx/helpers/transcoder.h"

using namespace boost::program_options;
using namespace Transport;

using namespace log4cxx;

NetworkPlugin * np = NULL;

int main (int argc, char* argv[]) {
	std::string host;
	int port;


	boost::program_options::options_description desc("Usage: spectrum [OPTIONS] <config_file.cfg>\nAllowed options");
	desc.add_options()
		("host,h", value<std::string>(&host), "host")
		("port,p", value<int>(&port), "port")
		;
	try
	{
		boost::program_options::variables_map vm;
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
		boost::program_options::notify(vm);
	}
	catch (std::runtime_error& e)
	{
		std::cout << desc << "\n";
		exit(1);
	}
	catch (...)
	{
		std::cout << desc << "\n";
		exit(1);
	}


	if (argc < 5) {
		qDebug("Usage: %s <config>", argv[0]);
		return 1;
	}

// 	QStringList channels;
// 	for (int i = 3; i < argc; ++i)
// 	{
// 		channels.append(argv[i]);
// 	}
// 
// 	MyIrcSession session;
// 	session.setNick(argv[2]);
// 	session.setAutoJoinChannels(channels);
// 	session.connectToServer(argv[1], 6667);

	Config config;
	if (!config.load(argv[5])) {
		std::cerr << "Can't open " << argv[1] << " configuration file.\n";
		return 1;
	}
	QCoreApplication app(argc, argv);

	if (CONFIG_STRING(&config, "logging.backend_config").empty()) {
		LoggerPtr root = log4cxx::Logger::getRootLogger();
#ifndef _MSC_VER
		root->addAppender(new ConsoleAppender(new PatternLayout("%d %-5p %c: %m%n")));
#else
		root->addAppender(new ConsoleAppender(new PatternLayout(L"%d %-5p %c: %m%n")));
#endif
	}
	else {
		log4cxx::helpers::Properties p;
		log4cxx::helpers::FileInputStream *istream = new log4cxx::helpers::FileInputStream(CONFIG_STRING(&config, "logging.backend_config"));
		p.load(istream);
		LogString pid, jid;
		log4cxx::helpers::Transcoder::decode(boost::lexical_cast<std::string>(getpid()), pid);
		log4cxx::helpers::Transcoder::decode(CONFIG_STRING(&config, "service.jid"), jid);
#ifdef _MSC_VER
		p.setProperty(L"pid", pid);
		p.setProperty(L"jid", jid);
#else
		p.setProperty("pid", pid);
		p.setProperty("jid", jid);
#endif
		log4cxx::PropertyConfigurator::configure(p);
	}

	Swift::QtEventLoop eventLoop;

	if (config.getUnregistered().find("service.irc_server") == config.getUnregistered().end()) {
		np = new IRCNetworkPlugin(&config, &eventLoop, host, port);
	}
	else {
		np = new SingleIRCNetworkPlugin(&config, &eventLoop, host, port);
	}

	return app.exec();
}
