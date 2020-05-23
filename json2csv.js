const moment = require('moment');
 
//remove cols, rows name and last "geschweifte Klammer"
const myData = [{"c":[{"v":"Date(2020,4,01)","f":null},{"v":220,"f":null}]},{"c":[{"v":"Date(2020,4,02)","f":null},{"v":80,"f":null}]},{"c":[{"v":"Date(2020,4,03)","f":null},{"v":640,"f":null}]},{"c":[{"v":"Date(2020,4,04)","f":null},{"v":260,"f":null}]},{"c":[{"v":"Date(2020,4,05)","f":null},{"v":20,"f":null}]},{"c":[{"v":"Date(2020,4,06)","f":null},{"v":340,"f":null}]},{"c":[{"v":"Date(2020,4,07)","f":null},{"v":420,"f":null}]},{"c":[{"v":"Date(2020,4,08)","f":null},{"v":540,"f":null}]},{"c":[{"v":"Date(2020,4,09)","f":null},{"v":700,"f":null}]},{"c":[{"v":"Date(2020,4,10)","f":null},{"v":500,"f":null}]},{"c":[{"v":"Date(2020,4,11)","f":null},{"v":40,"f":null}]},{"c":[{"v":"Date(2020,4,12)","f":null},{"v":120,"f":null}]},{"c":[{"v":"Date(2020,4,13)","f":null},{"v":20,"f":null}]},{"c":[{"v":"Date(2020,4,14)","f":null},{"v":40,"f":null}]},{"c":[{"v":"Date(2020,4,15)","f":null},{"v":80,"f":null}]},{"c":[{"v":"Date(2020,4,16)","f":null},{"v":420,"f":null}]},{"c":[{"v":"Date(2020,4,17)","f":null},{"v":820,"f":null}]},{"c":[{"v":"Date(2020,4,18)","f":null},{"v":380,"f":null}]},{"c":[{"v":"Date(2020,4,19)","f":null},{"v":440,"f":null}]},{"c":[{"v":"Date(2020,4,20)","f":null},{"v":340,"f":null}]},{"c":[{"v":"Date(2020,4,21)","f":null},{"v":660,"f":null}]},{"c":[{"v":"Date(2020,4,22)","f":null},{"v":240,"f":null}]},{"c":[{"v":"Date(2020,4,23)","f":null},{"v":0,"f":null}]},{"c":[{"v":"Date(2020,4,24)","f":null},{"v":0,"f":null}]},{"c":[{"v":"Date(2020,4,25)","f":null},{"v":0,"f":null}]},{"c":[{"v":"Date(2020,4,26)","f":null},{"v":0,"f":null}]},{"c":[{"v":"Date(2020,4,27)","f":null},{"v":0,"f":null}]},{"c":[{"v":"Date(2020,4,28)","f":null},{"v":0,"f":null}]},{"c":[{"v":"Date(2020,4,29)","f":null},{"v":0,"f":null}]},{"c":[{"v":"Date(2020,4,30)","f":null},{"v":0,"f":null}]},{"c":[{"v":"Date(2020,4,31)","f":null},{"v":0,"f":null}]}];

myData.forEach(element => {
    let date = element.c[0].v;
    let dataObj = eval("new "+date);
    let sum = element.c[1].v;

    console.log(moment(dataObj).format("YYYY.MM.DD")+","+sum);
});
