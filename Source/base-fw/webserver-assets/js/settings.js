
function AppLoaded()
{
  fetch('./api/getsettingsjson')
    .then((response) => response.json())
    .then((data) =>
    {
      console.log(data);
      currentApp.entries = data.entries;
    });
}

var currentApp = new Vue({
    el: '#app',
    data:
    {
        entries: [],
    }
  })
