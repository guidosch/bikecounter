$.ajax({
    url: "library/json_netsales.php",
    dataType: "json",
    success: function (json) {
        var data1 = new google.visualization.DataTable(json);

        // Instantiate and draw our chart, passing in some options.
        var chart1 = new google.visualization.AreaChart(document.getElementById('chart_netsales'));

        var options = {
            width: 300,
            height: '100%',
            title: 'Motivation Level Throughout the Day',
            hAxis: {
                title: 'Time of Day',
                format: 'h:mm a',
                viewWindow: {
                    min: [7, 30, 0],
                    max: [17, 30, 0]
                }
            },
            vAxis: {
                title: 'Rating (scale of 1-10)'
            },
            chartArea: {
                width: "100%",
                height: "80%"
            },
            legend: { position: 'none' },
            backgroundColor: '#232323',
            colors: ['#fff']
        }

        chart1.draw(data1, options);
    }
});