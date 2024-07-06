const express = require("express");
const bodyParser = require("body-parser");
const cors = require("cors");
const csv = require("csvtojson");
const fs = require("fs").promises; // For file system operations
const path = require("path");
require("./con_to_db"); // This ensures mongoose connects on startup
const Data = require("./dataModel");

const app = express();
const port = 3000;

app.use(
  cors({
    origin: "http://localhost:8080",
    methods: ["GET", "POST"],
  })
);
app.use(bodyParser.json());

app.post("/data", async (req, res) => {
  console.log(req.body);

  const dataString = req.body.data;
  const dataArray = dataString.split(" ");

  // Ensure that the received data has the correct number of elements
  if (dataArray.length !== 7) {
    return res.status(400).send("Invalid data format");
  }

  const newData = new Data({
    humidity: parseFloat(dataArray[0]),
    temperature: parseFloat(dataArray[1]),
    lpg: parseFloat(dataArray[2]),
    co: parseFloat(dataArray[3]),
    smoke: parseFloat(dataArray[4]),
    latitude: parseFloat(dataArray[5]),
    longitude: parseFloat(dataArray[6]),
  });

  try {
    const savedData = await newData.save();
    res.status(200).send(`Data inserted with id: ${savedData._id}`);
  } catch (error) {
    console.error(error);
    res.status(500).send("Error inserting data");
  }
});

app.get("/fire-data", async (req, res) => {
   
});

app.get("/device-data", async (req, res) => {
  try {
    const deviceData = await Data.find(); // Fetch all data from MongoDB
    
    res.json(deviceData); // Send data as JSON response
  } catch (error) {
    console.error(error);
    res.status(500).send("Error fetching device data");
  }
});


app.listen(port, () => {
  
  console.log(`Server running at http://localhost:${port}/`);
});
