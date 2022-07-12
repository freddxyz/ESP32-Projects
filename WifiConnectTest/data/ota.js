console.log("hello!");

var ota_path = "/ota_upload"

function upload() {
    var file_input = document.getElementById("firmware_file").files; 

    var xhttp = new XMLHttpRequest();

    xhttp.open("POST", ota_path, true);
    xhttp.send(file_input[0]);
}
