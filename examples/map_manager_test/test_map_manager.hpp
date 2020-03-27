#ifndef __TEST_SIMPLE_MAP__
#define __TEST_SIMPLE_MAP__

#include "vvs.h"
#include "dg_map_manager.hpp"
#include <stdint.h>
#include <cstdint>

int testSimpleMapManager()
{
	dg::MapManager manager;

	// Change the server IP address
	manager.setIP("129.254.87.96");
	std::string ip = manager.getIP();
	manager.setIP("localhost");
	ip = manager.getIP();
		
	// Get the path & map
	dg::Path path = manager.getPath(36.381873, 127.36803, 36.384063, 127.374733);
	VVS_CHECK_EQUL(manager.getPath().pts.size(), 37);
	VVS_CHECK_EQUL(manager.getMap().nodes.size(), 195);
	
    // Get the map
	VVS_CHECK_EQUL(manager.getMap(36.382967999999998, 127.37138150000001, 700).nodes.size(), 795);// 36.384063, 127.374733, 650.0));//

	// Get the path
	path = manager.getPath("test_simple_path.json");
	VVS_CHECK_EQUL(path.pts.size(), 37);
	
	// Find the node & edge
	dg::Node* findNode = manager.getMap().findNode(559542564800095);
	VVS_CHECK_EQUL(findNode->id, 559542564800095);
	dg::Edge* findEdge = manager.getMap().findEdge(559542564800095, 559542564800098);// 559562564900154, 559562564900155);
	VVS_CHECK_EQUL(findEdge->length, 34.39032729690151);// 13.370689140764002);
	VVS_CHECK_EQUL((manager.getMap().findEdge(findEdge->id))->length, 34.39032729690151);// 13.370689140764002);

	// Get the POI
	VVS_CHECK_EQUL(manager.getPOI(36.384063, 127.374733, 650.0).size(), 613);	// 36.382057170000003, 127.36764620000000, 10000.0, ip).size(), 8815);
	VVS_CHECK_EQUL(manager.getPOI(559542564800095, 500.0).size(), 212);
	VVS_CHECK_EQUL(manager.getPOI(16099168).lat, 36.378127999999997);
	//std::vector<cv::Point2d> poiloc = manager.getPOIloc("UST");
	
	// Get the StreetView
	VVS_CHECK_EQUL(manager.getStreetView(36.384063, 127.374733, 650.0).size(), 2144);	// 36.382057170000003, 127.36764620000000, 10000.0, ip).size(), 36607);
	VVS_CHECK_EQUL(manager.getStreetView(559542564800095, 500.0).size(), 662);
	VVS_CHECK_EQUL(manager.getStreetView(32364501511).heading, 36.378223300000002);

	// Get the StreetView image
	cv::Mat sv_image = manager.getStreetViewImage(14255003037, "");	// f, b, l, r, u, d

    return 0;
}

#endif // End of '__TEST_SIMPLE_MAP__'
