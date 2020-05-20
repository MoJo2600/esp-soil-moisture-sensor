FROM node:12-alpine AS build-stage
WORKDIR /app
COPY ./ /app/
RUN apk add python
RUN yarn install && yarn build
