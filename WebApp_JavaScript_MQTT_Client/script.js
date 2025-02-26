/*****************************************************
 * 1) Update your HTML to load AWS IoT Device SDK
 *****************************************************/
// Example:
// <script src="https://sdk.amazonaws.com/js/aws-sdk-2.814.0.min.js"></script>
// <script src="https://unpkg.com/aws-iot-device-sdk/dist/aws-iot-device-sdk.min.js"></script>
// <script src="app.js"></script>


/*****************************************************
 * 2) JavaScript Code (app.js)
 *****************************************************/

// AWS IoT WebSocket Endpoint (Ensure it's correct)
const AWS_IOT_ENDPOINT = "";

/**
 * Enhanced debug logging function
 */
function debugLog(type, message, data = null) {
  const timestamp = new Date().toISOString();
  const logPrefix = {
    info: "ℹ️ INFO",
    success: "✅ SUCCESS",
    warn: "⚠️ WARNING",
    error: "❌ ERROR",
    debug: "🔍 DEBUG",
  }[type] || "🔄 LOG";
  
  if (data) {
    console.log(`${timestamp} | ${logPrefix} | ${message}`, data);
  } else {
    console.log(`${timestamp} | ${logPrefix} | ${message}`);
  }
}

/**
 * Load AWS SDK if missing
 */
function loadAWSSDK(callback) {
  if (typeof AWS !== "undefined") {
    debugLog("info", "AWS SDK already loaded");
    callback();
    return;
  }

  debugLog("info", "Loading AWS SDK...");
  const script = document.createElement("script");
  script.src = "https://sdk.amazonaws.com/js/aws-sdk-2.814.0.min.js";
  script.onload = () => {
    debugLog("success", "AWS SDK loaded successfully");
    callback();
  };
  script.onerror = (err) => {
    debugLog("error", "Failed to load AWS SDK", err);
  };
  document.head.appendChild(script);
}

/**
 * Load AWS IoT Device SDK if missing
 */
function loadAWSIoTDeviceSDK(callback) {
  if (typeof awsIot !== "undefined") {
    debugLog("info", "aws-iot-device-sdk already loaded");
    callback();
    return;
  }

  debugLog("info", "Loading AWS IoT Device SDK...");
  const script = document.createElement("script");
  script.src = "https://unpkg.com/aws-iot-device-sdk/dist/aws-iot-device-sdk.min.js";
  script.onload = () => {
    debugLog("success", "AWS IoT Device SDK loaded successfully");
    callback();
  };
  script.onerror = (err) => {
    debugLog("error", "Failed to load AWS IoT Device SDK", err);
  };
  document.head.appendChild(script);
}

// Start the initialization process
loadAWSSDK(() => {
  loadAWSIoTDeviceSDK(() => {
    initializeAWS();
  });
});

function initializeAWS() {
  debugLog("info", "Initializing AWS configuration");
  
  // Log AWS SDK version
  debugLog("debug", `AWS SDK Version: ${AWS.VERSION}`);

  // Configure AWS Region
  const region = "sa-east-1";
  AWS.config.region = region;
  debugLog("debug", `AWS Region set to: ${region}`);
  
  // Configure Cognito Identity Pool
  const identityPoolId = "sa-east-1:03604983-2276-4ad6-9f1c-09efccfb9faa";
  debugLog("info", `Using Cognito Identity Pool: ${identityPoolId}`);

  AWS.config.credentials = new AWS.CognitoIdentityCredentials({
    IdentityPoolId: identityPoolId,
  });

  // Validate identity pool region
  if (!identityPoolId.includes(region)) {
    debugLog("warn", "Identity Pool region might not match configured AWS region");
  }

  // Refresh credentials, then connect to MQTT after they're loaded
  debugLog("info", "Refreshing AWS credentials...");
  AWS.config.credentials.clearCachedId();
  AWS.config.credentials.get((err) => {
    if (err) {
      debugLog("error", "Failed to load AWS Cognito credentials", err);
      debugLog("debug", "Cognito Error Details", {
        code: err.code,
        message: err.message,
        time: new Date().toISOString(),
      });
      return;
    }

    const creds = AWS.config.credentials;
    debugLog("success", "AWS Cognito credentials loaded successfully");

    // Debug credential details (safely)
    debugLog("debug", "Credential Details", {
      accessKeyId: creds.accessKeyId
        ? `${creds.accessKeyId.substr(0, 5)}...${creds.accessKeyId.substr(-4)}`
        : undefined,
      secretAccessKey: creds.secretAccessKey ? "Present (Hidden)" : undefined,
      sessionToken: creds.sessionToken
        ? `${creds.sessionToken.substr(0, 10)}...`
        : undefined,
      expiration: creds.expireTime,
      identityId: creds.identityId,
    });

    // Ensure credentials are valid
    if (!creds.accessKeyId || !creds.secretAccessKey || !creds.sessionToken) {
      debugLog("error", "AWS credentials are missing or invalid", {
        hasAccessKey: !!creds.accessKeyId,
        hasSecretKey: !!creds.secretAccessKey,
        hasSessionToken: !!creds.sessionToken,
      });
      return;
    }

    // Wait before connecting (demo delay)
    debugLog("info", "Waiting 3 seconds before connecting to MQTT...");
    setTimeout(() => {
      connectToAWSIoTDevice(creds);
    }, 3000);
  });
}

/**
 * Connect to AWS IoT using the official AWS IoT Device SDK
 */
function connectToAWSIoTDevice(creds) {
  // Validate endpoint
  if (!AWS_IOT_ENDPOINT || !AWS_IOT_ENDPOINT.includes("iot.")) {
    debugLog("error", "Invalid AWS IoT endpoint", AWS_IOT_ENDPOINT);
    return;
  }
  
  debugLog("info", `Preparing to connect to AWS IoT endpoint: ${AWS_IOT_ENDPOINT}`);

  // Generate unique client ID
  const clientId = "web_mqtt_" + Math.random().toString(16).substr(2, 8);
  debugLog("debug", `Generated client ID: ${clientId}`);

  // Create the AWS IoT Device SDK client
  debugLog("info", "Creating AWS IoT device...");
  const device = awsIot.device({
    region: AWS.config.region,
    host: AWS_IOT_ENDPOINT,
    clientId: clientId,
    protocol: "wss",
    maximumReconnectTimeMs: 8000,
    
    // Pass Cognito credentials so the SDK can SigV4-sign the connection
    accessKeyId: creds.accessKeyId,
    secretKey: creds.secretAccessKey,
    sessionToken: creds.sessionToken,
  });

  let connectionAttempts = 0;
  const connectionStartTime = Date.now();

  // Handle successful connection
  device.on("connect", () => {
    const connectionTime = Date.now() - connectionStartTime;
    debugLog("success", `Successfully connected to AWS IoT! Connection time: ${connectionTime}ms`);
    
    // Test subscription
    const testTopic = `test/connection/${clientId}`;
    debugLog("info", `Testing subscription to topic: ${testTopic}`);

    device.subscribe(testTopic, { qos: 0 }, (err) => {
      if (err) {
        debugLog("error", "Failed to subscribe to test topic", err);
      } else {
        debugLog("success", `Successfully subscribed to test topic: ${testTopic}`);

        // Publish test message
        setTimeout(() => {
          const testMessage = JSON.stringify({
            test: true,
            time: new Date().toISOString(),
          });
          device.publish(testTopic, testMessage, { qos: 0 }, (pubErr) => {
            if (pubErr) {
              debugLog("error", "Failed to publish test message", pubErr);
            } else {
              debugLog("success", `Published test message to ${testTopic}`);
            }
          });
        }, 1000);
      }
    });
  });

  // Handle errors
  device.on("error", (err) => {
    debugLog("error", "MQTT Connection Error", err);

    const errorMessage = err.message || String(err);
    if (errorMessage.includes("Not authorized")) {
      debugLog("error", "AWS IoT authentication failed - IAM Policy Issue", {
        suggestion: "Check your Cognito role policy for iot:Connect permissions, etc.",
      });
    } else if (errorMessage.includes("expired") || errorMessage.includes("expiration")) {
      debugLog("error", "Credential expiration issue", {
        suggestion: "Refresh Cognito credentials before connecting.",
      });
    } else {
      debugLog("error", "General connection error", {
        rawError: String(err),
        stack: err.stack,
      });
    }

    // Test AWS IoT permissions directly (optional)
    testAWSIoTPermissions(AWS.config.credentials);
  });

  // Handle close/reconnect/offline
  device.on("close", () => {
    debugLog("warn", "MQTT Connection Closed", {
      attempts: connectionAttempts,
      totalTimeElapsed: `${Math.round((Date.now() - connectionStartTime) / 1000)}s`,
    });
  });

  device.on("reconnect", () => {
    connectionAttempts++;
    debugLog("info", `Reconnecting to AWS IoT (Attempt #${connectionAttempts})...`);
  });

  device.on("offline", () => {
    debugLog("warn", "MQTT Client is offline", {
      suggestion: "Check network connectivity and AWS IoT policy permissions",
    });
  });

  // Message handler
  device.on("message", (topic, payload) => {
    const messageStr = payload.toString();
    debugLog("info", `Received message on topic [${topic}]`, messageStr);
    try {
      const jsonMessage = JSON.parse(messageStr);
      debugLog("debug", "Message parsed as JSON", jsonMessage);
    } catch {
      // Not JSON or parse failed—ignore
    }
  });

  // Provide a helper to send commands
  function sendMQTTCommand(topic, command) {
    if (!device.connected) {
      debugLog("error", "Cannot send command - MQTT client not connected");
      return false;
    }

    try {
      const message = JSON.stringify(command);
      debugLog("info", `Sending message to topic: ${topic}`, command);

      device.publish(topic, message, { qos: 1 }, (err) => {
        if (err) {
          debugLog("error", `Failed to publish to ${topic}`, err);
        } else {
          debugLog("success", `Message successfully sent to ${topic}`);
        }
      });
      return true;
    } catch (err) {
      debugLog("error", "Error preparing MQTT command", err);
      return false;
    }
  }

  window.sendMQTTCommand = sendMQTTCommand;

  // Quick test function
  window.testMQTTConnection = function() {
    const diagnostics = {
      connected: device.connected,
      connectionState: device.connected ? "Connected" : "Disconnected",
      endpoint: AWS_IOT_ENDPOINT,
      clientId: clientId,
      reconnectCount: connectionAttempts,
    };

    debugLog("info", "MQTT Connection Test", diagnostics);

    if (device.connected) {
      const testTopic = `test/diagnostic/${clientId}`;
      sendMQTTCommand(testTopic, { test: true, timestamp: new Date().toISOString() });
    } else {
      debugLog("warn", "Cannot test - client not connected");
    }

    return diagnostics;
  };

  // Periodically check connection status (for demonstration)
  const statusInterval = setInterval(() => {
    const status = device.connected ? "✅ Connected" : "❌ Disconnected";
    debugLog("info", `MQTT Connection Status: ${status}`, {
      reconnectAttempts: connectionAttempts,
      elapsedTime: `${Math.round((Date.now() - connectionStartTime) / 1000)}s`,
    });

    // Example: forcibly recreate the connection if many attempts fail
    if (!device.connected && connectionAttempts > 5 && connectionAttempts % 5 === 0) {
      debugLog("info", "Attempting connection recovery...");
      if (connectionAttempts > 10) {
        debugLog("warn", "Connection still failing after multiple attempts. Forcing reconnection...");
        device.end(true, () => {
          debugLog("info", "Device ended. Reinitializing in 5 seconds...");
          clearInterval(statusInterval);
          setTimeout(() => {
            debugLog("info", "Refreshing credentials and reconnecting...");
            AWS.config.credentials.clearCachedId();
            AWS.config.credentials.get(() => {
              connectToAWSIoTDevice(AWS.config.credentials);
            });
          }, 5000);
        });
      }
    }
  }, 5000);
}

// Function to test AWS IoT permissions directly (unchanged)
function testAWSIoTPermissions(credentials) {
  debugLog("info", "Testing AWS IoT permissions directly...");

  try {
    const iot = new AWS.Iot({
      region: AWS.config.region,
      credentials: credentials,
    });

    iot.describeEndpoint({ endpointType: "iot:Data-ATS" }, (err, data) => {
      if (err) {
        debugLog("error", "AWS IoT permission test failed", {
          error: err.code,
          message: err.message,
        });
      } else {
        debugLog("success", "AWS IoT permission test succeeded", {
          endpoint: data.endpointAddress,
          matches: data.endpointAddress === AWS_IOT_ENDPOINT,
        });

        if (data.endpointAddress !== AWS_IOT_ENDPOINT) {
          debugLog("warn", "Using incorrect IoT endpoint", {
            current: AWS_IOT_ENDPOINT,
            actual: data.endpointAddress,
            suggestion: "Update AWS_IOT_ENDPOINT to match",
          });
        }
      }
    });

    const iam = new AWS.IAM({
      region: AWS.config.region,
      credentials: credentials,
    });

    iam.getUser({}, (err, data) => {
      if (err) {
        if (err.code === "AccessDenied" || err.message.includes("is not authorized")) {
          debugLog("info", "IAM test expected failure with Cognito - this is normal in many setups");
        } else {
          debugLog("error", "IAM test unexpected error", err);
        }
      } else {
        debugLog("success", "IAM getUser succeeded", data);
      }
    });
  } catch (err) {
    debugLog("error", "Failed to initialize AWS IoT test", err);
  }
}

// Optional: Test endpoint connectivity (unchanged)
window.testIoTEndpoint = function() {
  debugLog("info", `Testing HTTPS connectivity to ${AWS_IOT_ENDPOINT}...`);
  fetch(`https://${AWS_IOT_ENDPOINT}/ping`)
    .then((response) => {
      debugLog("success", `HTTPS connectivity test result: ${response.status}`, {
        status: response.status,
        statusText: response.statusText,
      });
    })
    .catch((err) => {
      debugLog("error", "HTTPS connectivity test failed", err);
    });

  debugLog("info", "Testing WebSocket connectivity...");
  try {
    const ws = new WebSocket(`wss://${AWS_IOT_ENDPOINT}/mqtt`);
    ws.onopen = () => {
      debugLog("success", "WebSocket connection established");
      setTimeout(() => ws.close(), 1000);
    };
    ws.onerror = (error) => {
      debugLog("error", "WebSocket connection failed", error);
    };
    ws.onclose = (event) => {
      debugLog("info", "WebSocket connection closed", {
        code: event.code,
        reason: event.reason,
      });
    };
  } catch (err) {
    debugLog("error", "WebSocket initialization failed", err);
  }
  return "Tests initiated, check console for results";
};

