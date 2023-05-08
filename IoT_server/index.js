const express = require('express');
const mongoose = require('mongoose');
const Data = require("./data");
const app = express();
const bodyParser = require('body-parser');
const { response } = require('express');

app.use(bodyParser.json());
app.use(express.static('public'));

mongoose.connect('mongodb+srv://mohsen_tech:mohsen0930@webprojectcluster.oipym.mongodb.net/DB_data?retryWrites=true&w=majority', {
    useNewUrlParser: true,
    useUnifiedTopology: true
}).then(() => console.log('db connected!')).catch((err) => console.log(err));

app.get('/status', async function (req, res) {
    const temp = await Data.findOne({ old: false });
    const isTrigger = !!(temp);
    if (isTrigger) {
        temp.old = true;
        await temp.save();
    }
    res.json({ "status": isTrigger });
})

app.post('/status', async function (req, res) {
    if (req.body.status) {
        const newData = new Data({
            chipID: req.body.chipID,
        })
        await newData.save();
        res.json(newData);
    }
    else
        res.send();
})

app.get('/list', async function (req, res) {
    const temp = await Data.find({});
    res.json(temp);
})

const port = 3000;
app.listen(port, () => console.log(`virtual server listening at http://localhost:${port}`));