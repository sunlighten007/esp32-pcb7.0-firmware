/* Style */
String style =
"<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
"input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
"#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
"#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
"form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
".btn{background:#3498db;color:#fff;cursor:pointer}</style>";

/* Login page */
String loginIndex = 
"<form name=loginForm>"
"<h1>Bridge portal</h1>"
"<input name=userid placeholder='User ID'> "
"<input name=pwd placeholder=Password type=Password> "
"<input type=submit onclick=check(this.form) class=btn value=Login></form>"
"<script>"
"function check(form) {"
"if(form.userid.value=='admin' && form.pwd.value=='admin')"
"{window.open('/serverIndex')}"
"else"
"{alert('Error Password or Username')}"
"}"
"</script>" + style;
 
/* Server Index Page */
String serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
"<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
"<label id='file-input' for='file'>   Choose file...</label>"
"<input type='submit' class=btn value='Update'>"
"<br><br>"
"<div id='prg'></div>"
"<br><div id='prgbar'><div id='bar'></div></div><br></form>"
"<script>"
"function sub(obj){"
"var fileName = obj.value.split('\\\\');"
"document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
"};"
"$('form').submit(function(e){"
"e.preventDefault();"
"var form = $('#upload_form')[0];"
"var data = new FormData(form);"
"$.ajax({"
"url: '/update',"
"type: 'POST',"
"data: data,"
"contentType: false,"
"processData:false,"
"xhr: function() {"
"var xhr = new window.XMLHttpRequest();"
"xhr.upload.addEventListener('progress', function(evt) {"
"if (evt.lengthComputable) {"
"var per = evt.loaded / evt.total;"
"$('#prg').html('progress: ' + Math.round(per*100) + '%');"
"$('#bar').css('width',Math.round(per*100) + '%');"
"}"
"}, false);"
"return xhr;"
"},"
"success:function(d, s) {"
"console.log('success!') "
"},"
"error: function (a, b, c) {"
"}"
"});"
"});"
"</script>" + style;

const char* host = "esp32";
String wifi_ssid = "";
String wifi_password ="";
int wifi_status = 0;
const char* temp="";
String mqtt_username="";
String mqtt_password="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJkYXRhIjp7ImVtYWlsSWQiOiJ5ZGhlZXJhanJhb0BnbWFpbC5jb20iLCJ1c2VySWQiOiIwOGFhY2IwNy02MGM1LTQ5ZjAtYjViZS00MTVjZjBjNzk4M2UiLCJmaXJzdE5hbWUiOiJEaGVlcmFqIiwibGFzdE5hbWUiOiJSYW8ifSwiaWF0IjoxNjY5MjgzOTE2LCJleHAiOjE4MjY5NjM5MTZ9.4JzSp6J6Bz5_s75nLGuveEsT6Na1BOr1AmsZspY_HCE";

void generate_serial_id();
void connect_to_wifi(String _ssid, String _wifi_password);
void saveWifiDetailsLocally(String ssid, String wifi_password);
void turnOnLight(boolean isIntLight,boolean isExtLight);
void wifiSetupOnStart();
void BLE_Services();
void setup();
void process_cmdR1(String command);
void send_data();
void send_info();
void send_raw_data();
void updateWifiStatus();
void bluetooth_data();
void datareceived();
void ota_over_web();
void fn(int t, int t2);
void setup_mqtt_connection(void);
