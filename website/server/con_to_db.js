
const mongoose = require("mongoose");

const DB = 'mongodb+srv://jamesright:*IUseMongoDB0123@cluster0.kgkdlfo.mongodb.net/fire_detection?retryWrites=true&w=majority&appName=Cluster0'

mongoose.connect(DB).then(()=>{
  console.log("Connected To Database");
}).catch((err)=>{
  console.log(err);
});
 