module.exports = {
  "configureWebpack": {
    "devtool": "source-map",
    "optimization": {
      "splitChunks": false
    }
  },
  "css": {
    "extract": false
  },
  "transpileDependencies": [
    "vuetify"
  ]
}