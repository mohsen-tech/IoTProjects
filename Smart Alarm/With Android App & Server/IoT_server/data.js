const mongoose = require('mongoose');
const dataSchema = new mongoose.Schema({
    chipID: { type: Number },
    old: {
        type: Boolean,
        default: false,
    }
}, { timestamps: true });
const Data = mongoose.model('data', dataSchema, 'data');
module.exports = Data;