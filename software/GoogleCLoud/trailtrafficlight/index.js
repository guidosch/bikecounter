const Parser = require('rss-parser');
let parser = new Parser({
    customFields: {
        item: ['status',
            'username',
            'category',
            'official',
            'description',
            'active'],
    }
});

let trailMap = new Map();
trailMap.set("hoeckler", 'https://www.trailforks.com/trails/hocklertrail-by-zuritrails/reports/rss/');
trailMap.set("adlisberg", 'https://www.trailforks.com/trails/biketrail-adlisberg/reports/rss');
trailMap.set("antennentrail", 'https://www.trailforks.com/trails/antennentrail/reports/rss');

let statusMap = new Map();
statusMap.set("1", "ampelgruen");
statusMap.set("2", "ampelgelb");
statusMap.set("3", "ampelgelb");
statusMap.set("4", "ampelrot");

let footerMap = new Map();
footerMap.set("1", "Alles offen. Happy Trails!");
footerMap.set("2", "Bitte beachte die Hinweise. Danke für deine Rücksichtnahme!");
footerMap.set("3", "Bitte beachte die Hinweise und fahre nicht bei Nässe und Schlamm. Danke für deine Rücksichtnahme!");
footerMap.set("4", "Bitte beachte die Hinweise und fahre nicht bei Nässe und Schlamm. Danke für deine Rücksichtnahme!");

let status = "1";
let description = "Keine Beschreibung gefunden / No description found.";

exports.trafficlight = (req, res) => {
//export function trafficlight(req, res) {
    let trail = req.query.trail;
    
    if (!trail){
        trail = "hoeckler";
    }

    (async () => {

        let feed = await parser.parseURL(trailMap.get(trail));

        for (let index = 0; index < feed.items.length; index++) {
            const item = feed.items[index];
            if (item.official === "1") {
                status = item.status;
                description = item.description;
                break;
            }
        }

        let response = `
    <!doctype html>
    <head>
        <meta charset="utf-8">
        <style>
        .css-ampel {
            width: 30px;
            height: 90px;
            border-radius: 6px;
            position: relative;
            background-color: black;
            zoom: 1.7;
            float: left;
            margin-right: 10px
        }

        .css-ampel span,
        .css-ampel:before,
        .css-ampel:after {
            content: "";
            color: white;
            position: absolute;
            border-radius: 15px;
            width: 22px;
            height: 22px;
            left: 4px;
        }

        .css-ampel:before {
            top: 6px;
            background-color: red;
            background-color: dimgrey;
        }

        .css-ampel:after {
            top: 34px;
            background-color: yellow;
            background-color: dimgrey;
        }

        .css-ampel span {
            top: 62px;
            background-color: green;
            background-color: dimgrey;
        }

        .ampelrot:before {
            background-color: red;
            box-shadow: 0 0 20px red;
        }

        .ampelgelb:after {
            background-color: yellow;
            box-shadow: 0 0 20px yellow;
        }

        .ampelgruen span {
            background-color: limegreen;
            box-shadow: 0 0 20px limegreen;
        }

        /***
        tooltip css
        **/

        .hover {
            color: red;
        }

        .tooltip {
            top: -15px;
            color: white;
            opacity: 0;
            /*
            position: absolute;
            */
            -webkit-transition: opacity 0.5s;
            -moz-transition: opacity 0.5s;
            -ms-transition: opacity 0.5s;
            -o-transition: opacity 0.5s;
            transition: opacity 0.5s;
            display: none;
        }

        .tooltip>img {
            max-width: 300px;
            height: auto;
        }

        .hover:hover .tooltip {
            opacity: 1;
            display: block;
        }
    </style>
    </head>
    <body>
        <h1>Trailstatus</h1>    
        <span class="css-ampel ${statusMap.get(status)}"><span></span></span>
        <span id="desc">${description}
        <br>${footerMap.get(status)}</span>
        <br><span style="font-size:x-small"></span><br>
        <img id="infoIcon" width="16px" height="16px" src="data:image/png;base64, iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAACXBIWXMAAA7EAAAOxAGVKw4bAAAB
    KklEQVQ4jaXTMUscURTF8d+OW+UTCFpJsAtECFoJi9gGUlvYBJFgIUQQ1OaApEiZLhvrFBL8BrJs
    bR1iF0QsUqawsAiLhTNmHGZA9F893r3nvvM47/U0SDKPDaxirty+xAjDJL/q/b2asI9P+Ih+c3DJ
    BF+xk+TmfkAp/oF3HcImp3ib5KYoNw5bxNdYxitcNWqr+AK9JC9x3mL7LMlS6fA71lqus9DH+xYx
    vEmyj78t7qDAhz4GLcXqhK1acxsrhf9RNfmdZCbJjLsI25jtmvxoClw8Q39V6Lb3GEYFjvDvCeIJ
    htVL/IzdRsMfbJfrPbxu1L8l2azyP8C8h3lP47jLejV8Csbj8WQwGJzgBRZ15z7BEOsPPlOd8jtv
    YsXdG6mSGuEoyc96/y1WxEyezOSZ8QAAAABJRU5ErkJggg==" title="Der Trailstatus mit der Ampel wird automatisch anhand vom aktuellsten offiziellen Trailforks Report bestimmt. Der Status ist also ohne Gewähr!"/>
    </body>
    <script>
        (function() {
            let height = document.getElementById("desc").offsetHeight;
            let icon = document.getElementById("infoIcon");
            let margin = 130 - height;
            icon.style.marginTop = margin + "px";
            let counter = 1;
            document.body.querySelectorAll('img').forEach(function (node) {
                if (node.id !== "infoIcon") {
                    var outerWrapper = document.createElement('span');
                    outerWrapper.appendChild(document.createTextNode(' [Bild'+ counter+']'))
                    outerWrapper.classList.add("hover");
                    var imgWrapper = document.createElement('span');
                    imgWrapper.classList.add("tooltip");
                    let parent = node.parentNode;
                    imgWrapper.appendChild(node);
                    outerWrapper.appendChild(imgWrapper);
                    parent.appendChild(outerWrapper);
                    counter++;
                }
            });
        })();
    </script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/iframe-resizer/4.3.2/iframeResizer.contentWindow.js"
    integrity="sha512-cJ7aOLpXbec1Km9craM6xL6UOdlWf9etIz7f+cwQv2tuarLm3PLb3dv3ZqIK++SE4ui+EE0nWqKB0dOaAOv9gQ=="
    crossorigin="anonymous" referrerpolicy="no-referrer">
    </script>
    </html>
    `;
    if (feed.items.length > 0){
        res.status(200).send(response);
    } else {
        res.status(404).send("<html><body><h3>Cloud not load trailstatus. Try again later...</h3></body></html>");
    }

    })();
}

