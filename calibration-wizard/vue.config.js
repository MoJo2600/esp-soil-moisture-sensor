module.exports = {
  productionSourceMap: process.env.NODE_ENV == 'production' ? false : true,
  transpileDependencies: [
    "vuetify"
  ]
}