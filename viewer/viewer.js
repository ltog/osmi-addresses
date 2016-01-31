var map = L.map('map').setView([47.25278, 8.78823], 17);

// osm.org base map
L.tileLayer('http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
	attribution: 'Map data & Imagery &copy; <a href="http://openstreetmap.org/copyright">OpenStreetMap</a> contributors</a>',
	maxZoom: 25,
	maxNativeZoom: 19
}).addTo(map);

// add osmi-addresses output
L.tileLayer.wms('http://localhost/cgi-bin/mapserv?',{
	layers: 'nearest_roads,connection_lines,nearest_points,interpolation,buildings,buildings_with_addresses,nodes_with_addresses_interpolated,nodes_with_addresses_defined,postal_code,entrances_deprecated,entrances,interpolation_errors,no_addr_street,street_not_found,place_not_found',
	format: 'image/png; mode=24bit',
	transparent: 'TRUE',
	version: '1.1.1',
	crs: L.CRS.EPSG4326,
	projection: 'EPSG:900913',
	//map: 'X', // replace X with absolute path to .map file if you can't use/want to overwrite default path (set in environment variable MS_MAPFILE)
	displayprojection: 'EPSG:4326',
	tileSize: 2048,
	maxZoom: 25
}).addTo(map);
