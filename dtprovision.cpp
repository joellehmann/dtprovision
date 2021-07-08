// Library #############################################################
// Autoprovisioning of Digital Twins ###################################
// Masterthesis ########################################################
// by Joel Lehmann #####################################################
// 08.07.2021 ##########################################################

#include "dtprovision.h"
#include <Arduino.h>
#include <ESP8266HTTPClient.h>

DigitalTwin::DigitalTwin()
{

}

// Maybe including Credentials 
bool DigitalTwin::init (WiFiClient &client, String ip, int telemetry, int devReg, int ditto, int zeit)
{
    _client = &client;
    serverIP = ip;
    telemetryPort = telemetry;
    devRegPort = devReg;
    dittoPort = ditto;
    ms = zeit;

    telemetryURL = ip + ":" + telemetryPort;
    devRegURL = ip + ":" + devRegPort;
    dittoURL = ip + ":" + dittoPort;

    return true;
}

int DigitalTwin::createHonoTenant (String tenant)
{
    HTTPClient http;
    honoTenant = tenant;
    String srv = devRegURL + "/v1/tenants/" + honoTenant;
    http.begin(*_client,srv);

    int response = http.POST("");
    Serial.println();
    Serial.println("###   Hono Tenant Provision   ######################################"); 
    Serial.println("POST to Server: "+srv);  
    Serial.print("HTTP Response code: ");
    Serial.println(response);

    http.end();
    delay(ms);
    return response;
}

int DigitalTwin::createHonoDevice (String ns, String device)
{
    HTTPClient http;
    honoNamespace = ns;
    honoDevice = device;
    String srv = devRegURL + "/v1/devices/" + honoTenant + "/" + honoNamespace + ":" + honoDevice;
    http.begin(*_client,srv);

    int response = http.POST("");
    Serial.println();
    Serial.println("###   Hono Device Provision   ######################################");  
    Serial.println("POST to Server: "+srv);  
    Serial.print("HTTP Response code: ");
    Serial.println(response);

    http.end();
    delay(ms);
    return response;
}

int DigitalTwin::createHonoCredentials (String pass)
{
    HTTPClient http;
    honoDevicePassword = pass;
    String srv = devRegURL + "/v1/credentials/" + honoTenant + "/" + honoNamespace + ":" + honoDevice;
    http.begin(*_client,srv);
    http.addHeader("Content-Type", "application/json");

    String jsonParse = "[{\"type\":\"hashed-password\",\"auth-id\":\"" + honoDevice + "\",\"secrets\":[{\"pwd-plain\":\"" + honoDevicePassword + "\"}]}]";

    int response = http.PUT(jsonParse);
    Serial.println();
    Serial.println("###   Hono Credential Provision   ##################################"); 
    Serial.println("PUT to Server: "+srv);  
    Serial.print("HTTP Response code: ");
    Serial.println(response);

    http.end();
    delay(ms);
    return response;
}

int DigitalTwin::createDittoPiggyback ()
{
    HTTPClient http;
    String srv = dittoURL + "/devops/piggyback/connectivity";
    http.begin(*_client,srv);
    http.addHeader("Content-Type", "application/json");
    http.setAuthorization("devops", "foobar");

    String jsonParse = "{\"targetActorSelection\":\"/system/sharding/connection\",\"headers\":{\"aggregate\":false},\"piggybackCommand\":{\"type\":\"connectivity.commands:createConnection\",\"connection\":{\"id\":\"hono-connection-for-" + honoTenant + "\",\"connectionType\":\"amqp-10\",\"connectionStatus\":\"open\",\"uri\":\"amqp://consumer%40HONO:verysecret@c2e-dispatch-router-ext:15672\",\"failoverEnabled\":true,\"sources\":[{\"addresses\":[\"telemetry/" + honoTenant + "\",\"event/" + honoTenant + "\"],\"authorizationContext\":[\"pre-authenticated:hono-connection\"],\"enforcement\":{\"input\":\"{{header:device_id}}\",\"filters\":[\"{{entity:id}}\"]},\"headerMapping\":{\"hono-device-id\":\"{{header:device_id}}\",\"content-type\":\"{{header:content-type}}\"},\"replyTarget\":{\"enabled\":true,\"address\":\"{{header:reply-to}}\",\"headerMapping\":{\"to\":\"command/" + honoTenant + "/{{header:hono-device-id}}\",\"subject\":\"{{header:subject|fn:default(topic:action-subject)|fn:default(topic:criterion)}}-response\",\"correlation-id\":\"{{header:correlation-id}}\",\"content-type\":\"{{header:content-type|fn:default(\'application/vnd.eclipse.ditto+json\')}}\"},\"expectedResponseTypes\":[\"response\",\"error\"]},\"acknowledgementRequests\":{\"includes\":[],\"filter\":\"fn:filter(header:qos,\'ne\',\'0\')\"}},{\"addresses\":[\"command_response/" + honoTenant + "/replies\"],\"authorizationContext\":[\"pre-authenticated:hono-connection\"],\"headerMapping\":{\"content-type\":\"{{header:content-type}}\",\"correlation-id\":\"{{header:correlation-id}}\",\"status\":\"{{header:status}}\"},\"replyTarget\":{\"enabled\":false,\"expectedResponseTypes\":[\"response\",\"error\"]}}],\"targets\":[{\"address\":\"command/" + honoTenant + "\",\"authorizationContext\":[\"pre-authenticated:hono-connection\"],\"topics\":[\"_/_/things/live/commands\",\"_/_/things/live/messages\"],\"headerMapping\":{\"to\":\"command/" + honoTenant + "/{{thing:id}}\",\"subject\":\"{{header:subject|fn:default(topic:action-subject)}}\",\"content-type\":\"{{header:content-type|fn:default(\'application/vnd.eclipse.ditto+json\')}}\",\"correlation-id\":\"{{header:correlation-id}}\",\"reply-to\":\"{{fn:default(\'command_response/" + honoTenant + "/replies\')|fn:filter(header:response-required,\'ne\',\'false\')}}\"}},{\"address\":\"command/" + honoTenant + "\",\"authorizationContext\":[\"pre-authenticated:hono-connection\"],\"topics\":[\"_/_/things/twin/events\",\"_/_/things/live/events\"],\"headerMapping\":{\"to\":\"command/" + honoTenant + "/{{thing:id}}\",\"subject\":\"{{header:subject|fn:default(topic:action-subject)}}\",\"content-type\":\"{{header:content-type|fn:default(\'application/vnd.eclipse.ditto+json\')}}\",\"correlation-id\":\"{{header:correlation-id}}\"}}]}}}";

    int response = http.POST(jsonParse);
    Serial.println();
    Serial.println("###   Ditto Piggyback Provision   ##################################"); 
    Serial.println("POST to Server: "+srv);  
    Serial.print("HTTP Response code: ");
    Serial.println(response);

    http.end();
    delay(ms);
    return response;
}

int DigitalTwin::createDittoPolicy (String json)
{
    HTTPClient http;
    String srv = dittoURL + "/api/2/policies/" + honoNamespace + ":" + honoDevice;
    http.begin(*_client,srv);
    http.addHeader("Content-Type", "application/json");
    http.setAuthorization("ditto", "ditto");

    String jsonParse = json;

    int response = http.PUT(jsonParse);
    Serial.println();
    Serial.println("###   Ditto Policy Provision   #####################################"); 
    Serial.println("PUT to Server: "+srv);  
    Serial.print("HTTP Response code: ");
    Serial.println(response);

    http.end();
    delay(ms);
    return response;
}

int DigitalTwin::createDittoThing (String json)
{
    HTTPClient http;
    String srv = dittoURL + "/api/2/things/" + honoNamespace + ":" + honoDevice;
    http.begin(*_client,srv);
    http.addHeader("Content-Type", "application/json");
    http.setAuthorization("ditto", "ditto");

    String jsonParse = json;

    int response = http.PUT(jsonParse);
    Serial.println();
    Serial.println("###   Ditto Thing Provision   ######################################");
    Serial.println("PUT to Server: "+srv);  
    Serial.print("HTTP Response code: ");
    Serial.println(response);

    http.end();
    delay(ms);
    return response;
}

int DigitalTwin::createDittoFeatures (String json)
{
    HTTPClient http;
    String srv = dittoURL + "/api/2/things/" + honoNamespace + ":" + honoDevice + "/features";
    http.begin(*_client,srv);
    http.addHeader("Content-Type", "application/json");
    http.setAuthorization("ditto", "ditto");

    String jsonParse = json;

    int response = http.PUT(jsonParse);
    Serial.println();
    Serial.println("###   Ditto Features Provision   ###################################");
    Serial.println("PUT to Server: "+srv);  
    Serial.print("HTTP Response code: ");
    Serial.println(response);

    http.end();
    delay(ms);
    return response;
}



