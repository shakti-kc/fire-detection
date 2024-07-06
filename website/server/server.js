const express = require("express");
const bodyParser = require("body-parser");
const cors = require("cors");
const csv = require("csvtojson");
const fs = require("fs").promises;
const path = require("path");
require("./con_to_db");
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
  try {
    const response = await fetch(
      "https://firms.modaps.eosdis.nasa.gov/api/country/csv/API_KEY/VIIRS_SNPP_NRT/NPL/7"
    );
    const csvData = await response.text();
    const jsonData = await csv().fromString(csvData);

    const filePath = "E:\\Fire Detection System\\website\\public\\fire_risk.geojson";
    const geojsonContent = {
      type: "FeatureCollection",
      features: jsonData.map((item) => ({
        type: "Feature",
        geometry: {
          type: "Point",
          coordinates: [parseFloat(item.longitude), parseFloat(item.latitude)],
        },
        properties: {
          country_id: item.country_id,
          latitude: item.latitude,
          longitude: item.longitude,
          bright_ti4: item.bright_ti4,
          scan: item.scan,
          track: item.track,
          acq_date: item.acq_date,
          acq_time: item.acq_time,
          satellite: item.satellite,
          instrument: item.instrument,
          confidence: item.confidence,
          version: item.version,
          bright_ti5: item.bright_ti5,
          frp: item.frp,
          daynight: item.daynight,
        },
      })),
    };

    await fs.writeFile(filePath, JSON.stringify(geojsonContent));
    res.json({ msg: "success" });
  } catch (error) {
    console.error(error);
    res.status(500).send("Error fetching data");
  }
});

app.get("/device-data", async (req, res) => {
  try {
    const deviceData = await Data.find(); 
    
    res.json(deviceData);
  } catch (error) {
    console.error(error);
    res.status(500).send("Error fetching device data");
  }
});


app.listen(port, () => {
  console.log(`Server running at http://localhost:${port}/`);
});
