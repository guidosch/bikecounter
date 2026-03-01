return {
  data: {
    // Basic count from byte 0
    count: 24,
    
    // Version info from byte 1
    swVersion: 1,
    hwVersion: 2,
    
    // Status info from byte 2
    statId: 7,
    stat: "sync call",
    
    // Battery info calculated from byte 2
    batteryVoltage: 3.97,
    batteryLevel: 85,
    
    // Environmental data from byte 3
    temperature: 15.5,
    humidity: 71.4,
    
    // Timer configuration from byte 4
    intervalId: 2,
    selectedInterval: "< 4h",
    hourOfDay: 8,
    
    // Device time from bytes 5-7 (only if swVersion > 0)
    deviceTime: 1737715200,  // epoch timestamp in seconds
    timeDrift: 120,  // difference between server and device time in seconds
    
    // Array of timestamps when motion was detected
    timeArray: [
      1737715200000,
      1737718800000,
      1737722400000,
      // ... (count number of timestamps)
    ]
  },
  warnings: [],
  errors: []
}