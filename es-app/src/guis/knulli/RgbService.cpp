#include "guis/knulli/RgbService.h"
#include "utils/Platform.h"
#include "Paths.h"
#include "HttpReq.h"
#include <rapidjson/rapidjson.h>
#include <rapidjson/pointer.h>
#include <rapidjson/document.h>

#include "Log.h"

const std::string API_BASE_PATH = "http://localhost:1235/";
const std::string API_GET_MODES = "get-modes";
const std::string API_GET_PALETTES = "get-palettes";
const std::string API_RELOAD_CONFIG = "reload-config";
const std::string API_SET_CONFIG = "set-config";


void RgbService::reloadConfig()
{
	HttpReq* req = new HttpReq(API_BASE_PATH + API_RELOAD_CONFIG);
}

std::vector<ModeInfo> RgbService::getAvailableModes()
{

	LOG(LogError) << "RgbService::getAvailableModes called.";

	std::vector<ModeInfo> modes;

	LOG(LogError) << "Creating HttpReq for get-modes.";

	HttpReq* req = new HttpReq(API_BASE_PATH + API_GET_MODES);

	LOG(LogError) << "Waiting for HttpReq to complete.";

	if (req->wait())
	{
		rapidjson::Document doc;
		doc.Parse(req->getContent().c_str());
		if (doc.HasParseError())
			return modes;
		
		
		if (doc.IsObject() == false)
			return modes;

		for (auto& member : doc.GetObject())
		{

			if (member.name.IsString())
			{
				ModeInfo mode;
				mode.id = std::string(member.name.GetString());
				if (member.value.IsObject() && member.value.HasMember("name") && member.value["name"].IsString()) {
					mode.name = member.value["name"].GetString();
					modes.push_back(mode);
				}
			}
		}
		
	}

	return modes;
}

std::vector<PaletteInfo> RgbService::getAvailablePalettes()
{
	std::vector<PaletteInfo> palettes;

	HttpReq* req = new HttpReq(API_BASE_PATH + API_GET_PALETTES);

	if (req->wait())
	{
		rapidjson::Document doc;
		doc.Parse(req->getContent().c_str());
		if (doc.HasParseError())
			return palettes;
		
		
		if (doc.IsObject() == false)
			return palettes;

		for (auto& member : doc.GetObject())
		{

			if (member.name.IsString())
			{
				PaletteInfo palette;
				palette.id = std::string(member.name.GetString());
				if (member.value.IsObject() && member.value.HasMember("name") && member.value["name"].IsString()) {
					palette.name = member.value["name"].GetString();
					palettes.push_back(palette);
				}
			}
		}
		
	}

	return palettes;
}

void RgbService::applyValue(std::string key, std::string value)
{
	HttpReqOptions options;
	options.dataToPost = key + " " + value;
	
	HttpReq* req = new HttpReq(API_BASE_PATH + API_SET_CONFIG, &options);
	
	LOG(LogError) << "Posted config change to RgbService " << API_BASE_PATH + API_SET_CONFIG << " with data: " << options.dataToPost;
	
	if (req->wait())
	{
		// Optionally handle response
	}
}