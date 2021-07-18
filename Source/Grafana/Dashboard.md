# Grafana #

Below is the json representation of the Grafana dashboard for OpenKiln. You can import this into your Grafana installation. You will need a connection to your MSSQL server. I will get into the details of how to do that later.

http://sqlserver:3000/d/4ZgUh-MGz/kiln?orgId=1&refresh=5m&from=now-12h&to=now&kiosk
![Grafana-Trend](/Media/Grafana/Grafana-Trend.png)

```
{
  "annotations": {
    "list": [
      {
        "builtIn": 1,
        "datasource": "-- Grafana --",
        "enable": true,
        "hide": true,
        "iconColor": "rgba(0, 211, 255, 1)",
        "name": "Annotations & Alerts",
        "type": "dashboard"
      }
    ]
  },
  "editable": true,
  "gnetId": null,
  "graphTooltip": 0,
  "id": 6,
  "links": [],
  "panels": [
    {
      "datasource": null,
      "fieldConfig": {
        "defaults": {
          "custom": {},
          "mappings": [],
          "thresholds": {
            "mode": "absolute",
            "steps": [
              {
                "color": "green",
                "value": null
              }
            ]
          },
          "unit": "fahrenheit"
        },
        "overrides": []
      },
      "gridPos": {
        "h": 5,
        "w": 12,
        "x": 0,
        "y": 0
      },
      "id": 10,
      "options": {
        "colorMode": "value",
        "graphMode": "none",
        "justifyMode": "auto",
        "orientation": "auto",
        "reduceOptions": {
          "calcs": [
            "lastNotNull"
          ],
          "fields": "",
          "values": false
        }
      },
      "pluginVersion": "7.0.5",
      "targets": [
        {
          "alias": "",
          "format": "time_series",
          "rawSql": "DECLARE @TagIndex smallint\n\nSELECT @TagIndex = TagIndex\nFROM TagTable\nWHERE TagName = 'kiln_upper_temperature_01'\n\t\nSELECT\n  $__timeEpoch(DateAndTime),\n  Val as UpperTemperature\nFROM\n  FloatTable\nWHERE\n  $__timeFilter(DateAndTime) AND\n  TagIndex = @TagIndex\nORDER BY\n  DateAndTime ASC",
          "refId": "A"
        }
      ],
      "timeFrom": null,
      "timeShift": null,
      "title": "Upper Temperature",
      "type": "stat"
    },
    {
      "datasource": null,
      "fieldConfig": {
        "defaults": {
          "custom": {},
          "mappings": [],
          "thresholds": {
            "mode": "absolute",
            "steps": [
              {
                "color": "green",
                "value": null
              }
            ]
          },
          "unit": "fahrenheit"
        },
        "overrides": []
      },
      "gridPos": {
        "h": 5,
        "w": 12,
        "x": 12,
        "y": 0
      },
      "id": 11,
      "options": {
        "colorMode": "value",
        "graphMode": "none",
        "justifyMode": "auto",
        "orientation": "auto",
        "reduceOptions": {
          "calcs": [
            "lastNotNull"
          ],
          "fields": "",
          "values": false
        }
      },
      "pluginVersion": "7.0.5",
      "targets": [
        {
          "alias": "",
          "format": "time_series",
          "rawSql": "DECLARE @TagIndex smallint\n\nSELECT @TagIndex = TagIndex\nFROM TagTable\nWHERE TagName = 'kiln_lower_temperature_01'\n\t\nSELECT\n  $__timeEpoch(DateAndTime),\n  Val as UpperTemperature\nFROM\n  FloatTable\nWHERE\n  $__timeFilter(DateAndTime) AND\n  TagIndex = @TagIndex\nORDER BY\n  DateAndTime ASC",
          "refId": "A"
        }
      ],
      "timeFrom": null,
      "timeShift": null,
      "title": "Lower Temperature",
      "type": "stat"
    },
    {
      "aliasColors": {},
      "bars": false,
      "dashLength": 10,
      "dashes": false,
      "datasource": null,
      "fieldConfig": {
        "defaults": {
          "custom": {}
        },
        "overrides": []
      },
      "fill": 0,
      "fillGradient": 0,
      "gridPos": {
        "h": 10,
        "w": 24,
        "x": 0,
        "y": 5
      },
      "hiddenSeries": false,
      "id": 8,
      "legend": {
        "avg": false,
        "current": false,
        "max": false,
        "min": false,
        "show": true,
        "total": false,
        "values": false
      },
      "lines": true,
      "linewidth": 1,
      "nullPointMode": "null",
      "options": {
        "dataLinks": []
      },
      "percentage": false,
      "pointradius": 2,
      "points": false,
      "renderer": "flot",
      "seriesOverrides": [
        {
          "alias": "UpperTemperature"
        }
      ],
      "spaceLength": 10,
      "stack": false,
      "steppedLine": false,
      "targets": [
        {
          "alias": "",
          "format": "time_series",
          "rawSql": "DECLARE @TagIndex smallint\n\nSELECT @TagIndex = TagIndex\nFROM TagTable\nWHERE TagName = 'kiln_upper_temperature_01'\n\t\nSELECT\n  $__timeEpoch(DateAndTime),\n  Val as UpperTemperature\nFROM\n  FloatTable\nWHERE\n  $__timeFilter(DateAndTime) AND\n  TagIndex = @TagIndex\nORDER BY\n  DateAndTime ASC",
          "refId": "A"
        },
        {
          "alias": "",
          "format": "time_series",
          "rawSql": "DECLARE @TagIndex smallint\n\nSELECT @TagIndex = TagIndex\nFROM TagTable\nWHERE TagName = 'kiln_lower_temperature_01'\n\t\nSELECT\n  $__timeEpoch(DateAndTime),\n  Val as LowerTemperature\nFROM\n  FloatTable\nWHERE\n  $__timeFilter(DateAndTime) AND\n  TagIndex = @TagIndex\nORDER BY\n  DateAndTime ASC",
          "refId": "B"
        }
      ],
      "thresholds": [],
      "timeFrom": null,
      "timeRegions": [],
      "timeShift": null,
      "title": "Temperatures",
      "tooltip": {
        "shared": true,
        "sort": 0,
        "value_type": "individual"
      },
      "type": "graph",
      "xaxis": {
        "buckets": null,
        "mode": "time",
        "name": null,
        "show": true,
        "values": []
      },
      "yaxes": [
        {
          "format": "short",
          "label": null,
          "logBase": 1,
          "max": null,
          "min": null,
          "show": true
        },
        {
          "format": "short",
          "label": null,
          "logBase": 1,
          "max": null,
          "min": null,
          "show": true
        }
      ],
      "yaxis": {
        "align": false,
        "alignLevel": null
      }
    },
    {
      "datasource": null,
      "fieldConfig": {
        "defaults": {
          "custom": {},
          "mappings": [],
          "thresholds": {
            "mode": "absolute",
            "steps": [
              {
                "color": "green",
                "value": null
              }
            ]
          },
          "unit": "fahrenheit"
        },
        "overrides": []
      },
      "gridPos": {
        "h": 5,
        "w": 12,
        "x": 6,
        "y": 15
      },
      "id": 12,
      "options": {
        "colorMode": "value",
        "graphMode": "none",
        "justifyMode": "auto",
        "orientation": "auto",
        "reduceOptions": {
          "calcs": [
            "max"
          ],
          "fields": "",
          "values": false
        }
      },
      "pluginVersion": "7.0.5",
      "targets": [
        {
          "alias": "",
          "format": "time_series",
          "rawSql": "DECLARE @TagIndex smallint\n\nSELECT @TagIndex = TagIndex\nFROM TagTable\nWHERE TagName = 'kiln_lower_temperature_01'\n\t\nSELECT\n  $__timeEpoch(DateAndTime),\n  Val as UpperTemperature\nFROM\n  FloatTable\nWHERE\n  $__timeFilter(DateAndTime) AND\n  TagIndex = @TagIndex\nORDER BY\n  DateAndTime ASC",
          "refId": "A"
        }
      ],
      "timeFrom": null,
      "timeShift": null,
      "title": "Max Temperature",
      "type": "stat"
    }
  ],
  "refresh": false,
  "schemaVersion": 25,
  "style": "dark",
  "tags": [],
  "templating": {
    "list": []
  },
  "time": {
    "from": "2021-07-02T01:44:10.983Z",
    "to": "2021-07-03T08:22:55.455Z"
  },
  "timepicker": {
    "refresh_intervals": [
      "10s",
      "30s",
      "1m",
      "5m",
      "15m",
      "30m",
      "1h",
      "2h",
      "1d"
    ]
  },
  "timezone": "",
  "title": "Kiln",
  "uid": "4ZgUh-MGz",
  "version": 26
}
```