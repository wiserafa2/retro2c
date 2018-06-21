module.exports = [
  {
    "type": "heading",
    "defaultValue": "RetroII Configuration"
  },
{
  "type": "select",
  "messageKey": "APPLE_TYPE",
  "defaultValue": "2",
  "label": "Apple II version",
  "options": 
  [
    { 
      "label": " APPLE II ",
      "value": "2"
    },
    { 
      "label": " Apple //c ", 
      "value": "0"
    },
    { 
      "label": " Apple //e ",
      "value": "1"
    },
    { 
      "label": " Apple IIgs ",
      "value": "3"
    },
  ]
},   
    {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Toggle Settings"
      },
      {
        "type": "toggle",
        "messageKey": "USE_FAHRENHEIT",
        "label": "USE_FAHRENHEIT",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "SHOW_WEATHER",
        "label": "SHOW_WEATHER",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "SHOW_DATE",
        "label": "SHOW_DATE",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "SHOW_DISTANCE",
        "label": "SHOW_DISTANCE",
        "defaultValue": true
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];