#ifndef __FIREBASE_CONTROLLER_H__
#define __FIREBASE_CONTROLLER_H__

#include "mcp_server.h"
#include "board.h"
#include <esp_log.h>
#include <cJSON.h>

#define FB_TAG "FirebaseCtrl"

class FirebaseSmartHomeController {
private:
    static std::string GetFirebaseAuthToken() {
        auto& board = Board::GetInstance();
        auto network = board.GetNetwork();
        if (!network) return "";
        auto http = network->CreateHttp(0);
        if (!http) return "";

        http->SetHeader("Content-Type", "application/json");
        http->SetContent("{\"returnSecureToken\":true}");
        std::string auth_url = "https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=AIzaSyCnupBCvmT1P7ItpOTOCe0HGA1vovytwbQ";
        if (http->Open("POST", auth_url)) {
            std::string response = http->ReadAll();
            http->Close();
            
            cJSON* json = cJSON_Parse(response.c_str());
            if (json) {
                cJSON* idToken = cJSON_GetObjectItem(json, "idToken");
                if (idToken && idToken->valuestring) {
                    std::string token = idToken->valuestring;
                    cJSON_Delete(json);
                    return token;
                }
                cJSON_Delete(json);
            }
        }
        return "";
    }

    static bool UpdateFirebaseDevice(const std::string& device, const std::string& key, const std::string& val_str) {
        std::string token = GetFirebaseAuthToken();
        if (token.empty()) {
            ESP_LOGE(FB_TAG, "Failed to get Firebase auth token");
            return false;
        }
        
        auto& board = Board::GetInstance();
        auto network = board.GetNetwork();
        if (!network) return false;
        auto http = network->CreateHttp(0);
        if (!http) return false;

        http->SetHeader("Content-Type", "application/json");
        char body[128];
        snprintf(body, sizeof(body), "{\"%s\": %s}", key.c_str(), val_str.c_str());
        http->SetContent(std::string(body));
        
        std::string url = "https://esp32-smart-home-c7e8a-default-rtdb.asia-southeast1.firebasedatabase.app/devices/" + device + ".json?auth=" + token;
        if (http->Open("PATCH", url)) {
            ESP_LOGI(FB_TAG, "Firebase updated successfully: %s/%s -> %s", device.c_str(), key.c_str(), val_str.c_str());
            http->ReadAll();
            http->Close();
            return true;
        } else {
            ESP_LOGE(FB_TAG, "Failed to send HTTP PATCH to Firebase");
            return false;
        }
    }

public:
    FirebaseSmartHomeController() {
        auto& mcp_server = McpServer::GetInstance();

        // 1. Đèn trong nhà (Indoor Light)
        mcp_server.AddTool("self.indoor_light.turn_on", "Bật đèn trong nhà (Indoor Light)", PropertyList(), [](const PropertyList& properties) -> ReturnValue {
            return UpdateFirebaseDevice("indoor_light", "status", "true");
        });
        mcp_server.AddTool("self.indoor_light.turn_off", "Tắt đèn trong nhà (Indoor Light)", PropertyList(), [](const PropertyList& properties) -> ReturnValue {
            return UpdateFirebaseDevice("indoor_light", "status", "false");
        });

        // 2. Quạt thông minh (Smart Fan)
        mcp_server.AddTool("self.fan.turn_on", "Bật quạt thông minh", PropertyList(), [](const PropertyList& properties) -> ReturnValue {
            return UpdateFirebaseDevice("fan", "status", "true");
        });
        mcp_server.AddTool("self.fan.turn_off", "Tắt quạt thông minh", PropertyList(), [](const PropertyList& properties) -> ReturnValue {
            return UpdateFirebaseDevice("fan", "status", "false");
        });

        // 3. Đèn ngoài sân (Outdoor Light)
        mcp_server.AddTool("self.outdoor_light.turn_on", "Bật đèn ngoài sân (Outdoor Light)", PropertyList(), [](const PropertyList& properties) -> ReturnValue {
            UpdateFirebaseDevice("outdoor_light", "mode", "\"manual\"");
            return UpdateFirebaseDevice("outdoor_light", "status", "true");
        });
        mcp_server.AddTool("self.outdoor_light.turn_off", "Tắt đèn ngoài sân (Outdoor Light)", PropertyList(), [](const PropertyList& properties) -> ReturnValue {
            UpdateFirebaseDevice("outdoor_light", "mode", "\"manual\"");
            return UpdateFirebaseDevice("outdoor_light", "status", "false");
        });

        // 4. Mái che tự động (Smart Roof)
        mcp_server.AddTool("self.roof.open", "Mở mái che tự động (Smart Roof)", PropertyList(), [](const PropertyList& properties) -> ReturnValue {
            UpdateFirebaseDevice("roof", "mode", "\"manual\"");
            return UpdateFirebaseDevice("roof", "status", "\"open\"");
        });
        mcp_server.AddTool("self.roof.close", "Đóng mái che tự động (Smart Roof)", PropertyList(), [](const PropertyList& properties) -> ReturnValue {
            UpdateFirebaseDevice("roof", "mode", "\"manual\"");
            return UpdateFirebaseDevice("roof", "status", "\"closed\"");
        });
    }
};

#endif // __FIREBASE_CONTROLLER_H__
