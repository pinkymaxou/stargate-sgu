function idBtnUpload_Click()
{
	try {
		let lblStatus = document.getElementById("lblStatus");

		console.log("idBtnUpload_Click");

		var xhr = new XMLHttpRequest();
		xhr.open('POST', '/ota/upload', true);

		let firmwareFile = document.getElementById("idFile").files[0];

		// Listen to the upload progress.
		xhr.upload.onprogress = function(e)
		{
			lblStatus.innerHTML = "Uploading (" + ((e.loaded / e.total)*100).toFixed(2) + "%)";
		};

		xhr.onreadystatechange = function (e) {
			if (xhr.readyState === 4) {
				if (xhr.status === 200) {
					console.log("Upload succeeded!", e);
					lblStatus.innerHTML = "Upload completed!";
				} else {
					console.log("Error: ", e);
					lblStatus.innerHTML = "Upload error: " + xhr.statusText;
				}
			}
		};

		lblStatus.innerHTML = "Uploading ...";
		xhr.send(firmwareFile);
	}
	catch(err) {
		lblStatus.innerHTML = "Upload error: " + err.message;
	}
}

function AppLoaded()
{
  fetch('./api/getsysinfo')
    .then((response) => response.json())
    .then((data) =>
    {
      console.log(data);
      currentApp.sysinfos = data.infos;
    });
}

var currentApp = new Vue({
    el: '#app',
    data:
    {
      sysinfos: [],
    }
  })
