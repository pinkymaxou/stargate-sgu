function idBtnUpload_Click()
{
	console.log("idBtnUpload_Click");
	
	var xhr = new XMLHttpRequest();
	xhr.open('POST', '/ota/upload', true);

	let firmwareFile = document.getElementById("idFile").files[0];

	// Listen to the upload progress.
	xhr.upload.onprogress = function(e) 
	{
		console.log("upload in onprogress", e);
	};
	
	xhr.upload.loadstart = function(e)
	{
		console.log("upload started", e);	
	};

	xhr.upload.loadend = function(e)
	{
		console.log("upload ended", e);	
	};

	xhr.upload.load  = function(e)
	{
		console.log("upload succeeded!", e);	
	};

	xhr.upload.error = function(e)
	{
		console.log("upload error", e);	
	};

	xhr.upload.abort = function(e)
	{
		console.log("upload aborted", e);	
	};
	
	xhr.send(firmwareFile);
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
  