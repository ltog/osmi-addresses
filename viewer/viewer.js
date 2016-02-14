var map = L.map('map', {fadeAnimation: false}).setView([47.25278, 8.78823], 17);

// osm.org base map
L.tileLayer('http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
	attribution: 'Map data & Imagery &copy; <a href="http://openstreetmap.org/copyright">OpenStreetMap</a> contributors</a>',
	maxZoom: 25,
	maxNativeZoom: 19
}).addTo(map);

var wms_options = {
	format: 'image/png; mode=24bit',
	transparent: 'TRUE',
	version: '1.1.1',
	crs: L.CRS.EPSG4326,
	projection: 'EPSG:900913',
	//map: 'X', // replace X with absolute path to .map file if you can't use/want to overwrite default path (set in environment variable MS_MAPFILE)
	displayprojection: 'EPSG:4326'
}

var ms_source = L.WMS.source('http://localhost/cgi-bin/mapserv?', wms_options);

var buildings                         = ms_source.getLayer('buildings').addTo(map);
var buildings_with_addresses          = ms_source.getLayer('buildings_with_addresses').addTo(map);
var postal_code                       = ms_source.getLayer('postal_code').addTo(map);
var entrances_deprecated              = ms_source.getLayer('entrances_deprecated').addTo(map);
var entrances                         = ms_source.getLayer('entrances').addTo(map);
var no_addr_street                    = ms_source.getLayer('no_addr_street').addTo(map);
var street_not_found                  = ms_source.getLayer('street_not_found').addTo(map);
var place_not_found                   = ms_source.getLayer('place_not_found').addTo(map);
var misformatted_housenumber          = ms_source.getLayer('misformatted_housenumber').addTo(map);
var nodes_with_addresses_defined      = ms_source.getLayer('nodes_with_addresses_defined').addTo(map);
var nodes_with_addresses_interpolated = ms_source.getLayer('nodes_with_addresses_interpolated').addTo(map);
var interpolation                     = ms_source.getLayer('interpolation').addTo(map);
var interpolation_errors              = ms_source.getLayer('interpolation_errors').addTo(map);
var connection_lines                  = ms_source.getLayer('connection_lines').addTo(map);
var nearest_points                    = ms_source.getLayer('nearest_points').addTo(map);
var nearest_roads                     = ms_source.getLayer('nearest_roads').addTo(map);
var nearest_areas                     = ms_source.getLayer('nearest_areas').addTo(map);
var addrx_on_nonclosed_way            = ms_source.getLayer('addrx_on_nonclosed_way').addTo(map);

var control = L.control.layers({}, {
	"Buildings" : buildings,
	"Buildings with addresses" : buildings_with_addresses,
	"Ways with postal_code" : postal_code,
	"Entrances (old style)" : entrances_deprecated,
	"Entrances" : entrances,
	"No addr:street tag" : no_addr_street,
	"Street not found" : street_not_found,
	"Place not found" : place_not_found,
	"Misformatted housenumber" : misformatted_housenumber,
	"Defined addresses" : nodes_with_addresses_defined,
	"Interpolated addresses" : nodes_with_addresses_interpolated,
	"Interpolation lines without errors" : interpolation,
	"Interpolation lines with errors" : interpolation_errors,
	"Connection lines" : connection_lines,
	"Connection points" : nearest_points,
	"Nearest roads" : nearest_roads,
	"Nearest areas" : nearest_areas,
	"addr:* on non-closed way" : addrx_on_nonclosed_way
}, {
	collapsed: false
});

control.addTo(map);
