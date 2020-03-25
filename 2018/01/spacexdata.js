var plotLaunches = function(by_weekday_div, by_hour_div, data) {
    // Launches grouped by the orbit kind and the day of the week.
    by_weekday = {}

    // Launches grouped by the orbit kind and the hour.
    by_hour = {}
    $.each(data, function(index, info) {
        if (info['rocket']['rocket_id'] != 'falcon1') {
            // Convert the launch date to Hawthorne local time.
            var launch_date = new timezoneJS.Date(info['launch_date_unix'] * 1000.0);
            launch_date.setTimezone('US/Pacific');

            var orbit = info['rocket']['second_stage']['payloads'][0]['orbit'];

            if (!(orbit in by_weekday)) {
                by_weekday[orbit] = Array(7).fill(0);
            }

            // Fix getDay() up so that Monday becomes the first day of the week. 
            by_weekday[orbit][(launch_date.getDay() + 6) % 7] += 1;

            if (!(orbit in by_hour)) {
                by_hour[orbit] = Array(24).fill(0);
            }

            by_hour[orbit][launch_date.getHours()] += 1;
        }
    });

    /*
     * Format the plot showing launches by the day of the week.
     */

    var days = [
        'понедельник',
        'вторник',
        'среда',
        'четверг',
        'пятница',
        'суббота',
        'воскресенье'
    ];

    var plot_data = []

    $.each(by_weekday, function(orbit, launches) {
        plot_data.push({
            name: orbit,
            type: 'bar',
            x: days,
            y: launches
        });
    })

    var plot_layout = {
        barmode: 'stack',
        title: 'Запуски по дням недели',
        xaxis: {
            title: 'День недели',
        },
        yaxis: {
            title: 'Количество запусков',
        },
    };

    var by_weekday_node = Plotly.d3.select(by_weekday_div)
            .style({width: '100%'}).node();
    Plotly.newPlot(by_weekday_node, plot_data, plot_layout);

    /*
     * Format the plot showing launches by the hour of the day.
     */

    hours = Array.apply(null, Array(24)).map(function (x, i) { return i + ':00'; })

    var plot_data = []

    $.each(by_hour, function(orbit, launches) {
        plot_data.push({
            name: orbit,
            type: 'bar',
            x: hours,
            y: launches,
        });
    })

    var plot_layout = {
        barmode: 'stack',
        title: 'Запуски по времени дня',
        xaxis: {
            title: 'Время дня',
        },
        yaxis: {
            title: 'Количество запусков',
        },
    };

    var by_hour_node = Plotly.d3.select(by_hour_div)
            .style({width: '100%'}).node();
    Plotly.newPlot(by_hour_node, plot_data, plot_layout);

    window.onresize = function() {
        Plotly.Plots.resize(by_weekday_node);
        Plotly.Plots.resize(by_hour_node);
    };
};

timezoneJS.timezone.zoneFileBasePath = '/tz';
timezoneJS.timezone.init({
    callback: function() {
        $.getJSON('https://api.spacexdata.com/v2/launches', function(data) {
            plotLaunches('#launches_by_weekday', '#launches_by_hour', data);
        });
    }
});

