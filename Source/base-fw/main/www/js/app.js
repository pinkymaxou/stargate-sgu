function SendAction(actionURL, data)
{
  var xhr = new XMLHttpRequest();
  xhr.open("POST", actionURL, true);
  if (data) {
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send(JSON.stringify(data));
  }
  else {
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send(JSON.stringify({}));
  }
}

function DialGate(symbols)
{
  SendAction('action/dial', { symbols : symbols } );
}

let allsymbols = [
  { id: 1, imgfile: "img/glyph/01.png", txt: '\u0041' },
  { id: 2, imgfile: "img/glyph/02.png", txt: '\u0042' },
  { id: 3, imgfile: "img/glyph/03.png", txt: '\u0043' },
  { id: 4, imgfile: "img/glyph/04.png", txt: '\u0044' },
  { id: 5, imgfile: "img/glyph/05.png", txt: '\u0045' },
  { id: 6, imgfile: "img/glyph/06.png", txt: '\u0046' },

  { id: 7, imgfile: "img/glyph/07.png",  txt: '\u0047' },
  { id: 8, imgfile: "img/glyph/08.png",  txt: '\u0048' },
  { id: 9, imgfile: "img/glyph/09.png",  txt: '\u0049' },
  { id: 10, imgfile: "img/glyph/10.png", txt: '\u004A' },
  { id: 11, imgfile: "img/glyph/11.png", txt: '\u004B' },
  { id: 12, imgfile: "img/glyph/12.png", txt: '\u004C' },

  { id: 13, imgfile: "img/glyph/13.png", txt: '\u004D' },
  { id: 14, imgfile: "img/glyph/14.png", txt: '\u004E' },
  { id: 15, imgfile: "img/glyph/15.png", txt: '\u004F' },
  { id: 16, imgfile: "img/glyph/16.png", txt: '\u0050' },
  { id: 17, imgfile: "img/glyph/17.png", txt: '\u0051' },
  { id: 18, imgfile: "img/glyph/18.png", txt: '\u0052' },

  { id: 19, imgfile: "img/glyph/19.png", txt: '\u0053' },
  { id: 20, imgfile: "img/glyph/20.png", txt: '\u0054' },
  { id: 21, imgfile: "img/glyph/21.png", txt: '\u0055' },
  { id: 22, imgfile: "img/glyph/22.png", txt: '\u0056' },
  { id: 23, imgfile: "img/glyph/23.png", txt: '\u0057' },
  { id: 24, imgfile: "img/glyph/24.png", txt: '\u0058' },

  { id: 25, imgfile: "img/glyph/25.png", txt: '\u0059' },
  { id: 26, imgfile: "img/glyph/26.png", txt: '\u005A' },
  { id: 27, imgfile: "img/glyph/27.png", txt: '\u0031' },
  { id: 28, imgfile: "img/glyph/28.png", txt: '\u0032' },
  { id: 29, imgfile: "img/glyph/29.png", txt: '\u0033' },
  { id: 30, imgfile: "img/glyph/30.png", txt: '\u0034' },

  { id: 31, imgfile: "img/glyph/31.png", txt: '\u0035' },
  { id: 32, imgfile: "img/glyph/32.png", txt: '\u0036' },
  { id: 33, imgfile: "img/glyph/33.png", txt: '\u0037' },
  { id: 34, imgfile: "img/glyph/34.png", txt: '\u0038' },
  { id: 35, imgfile: "img/glyph/35.png", txt: '\u0039' },
  { id: 36, imgfile: "img/glyph/36.png", txt: '\u0030' },
];

let alladdresses = [
  { symbolIndexes: [10, 15, 20, 26, 28, 13, 18, 32, 30], name: 'Earth' },
  { symbolIndexes: [3, 34, 12, 7, 19, 6, 29],            name: 'Jungle Planet' },
  { symbolIndexes: [7, 8, 14, 17, 32, 23, 33],           name: 'Desert Planet' },
  { symbolIndexes: [15, 35, 8, 30, 31, 29, 33],          name: 'Hoth' },
  { symbolIndexes: [1, 34, 12, 7, 25, 32, 33],           name: 'Grave Pit Planet' },
  { symbolIndexes: [10, 12, 14, 23, 32, 23, 33],         name: 'Ruins Planet' },
  { symbolIndexes: [4, 20, 23, 28, 6, 11, 33],           name: 'Foggy Planet' },
  { symbolIndexes: [4, 17, 23, 32, 35, 8, 33],           name: 'Deportation Planet' },
  { symbolIndexes: [3, 13, 17, 19, 1, 35, 33],           name: 'Cloverdale Planet' },
  { symbolIndexes: [2, 14, 18, 23, 30, 5, 33],           name: 'Malice Planet' },
  { symbolIndexes: [5, 17, 23, 25, 1, 28, 33],           name: 'Novus Colony Planet' },
  { symbolIndexes: [6, 26, 28, 10, 31, 12, 33],          name: 'Last Planet' },
  { symbolIndexes: [1, 36, 2, 35, 3, 34],                name: 'Test #1' },
];

function GetSymbolIndex(SymbolOneBased)
{
  return allsymbols[SymbolOneBased-1];
}

let currentData =
{
  symbols: allsymbols,
  alladdresses: alladdresses,

  status: {
    text: "",
    cancel_request: false,
    time_hour: 0, time_min: 0
  }
};

var currentApp = new Vue({
  el: '#app',
  data: currentData
})

async function timerHandler() {

  // Get system informations
  await fetch('/api/getstatus')
      .then((response) => {
          if (!response.ok) {
              throw new Error('Unable to get status');
          }
          return response.json();
      })
      .then((data) =>
      {
        console.log("data: ", data);
        currentData.status = data.status;
        setTimeout(timerHandler, 500);
      })
      .catch((ex) =>
      {
          setTimeout(timerHandler, 5000);
          console.error('getstatus', ex);
      });
}

window.addEventListener(
  "load",
  (event) => {
    console.log("page is fully loaded");
    setTimeout(timerHandler, 500);
});
