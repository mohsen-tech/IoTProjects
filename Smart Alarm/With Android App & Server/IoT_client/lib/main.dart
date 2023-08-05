import 'package:flutter/material.dart';
import 'package:iot_client/app/app.dart';
import 'package:iot_client/core/service_locator.dart';

void main() {
  configureServiceLocator();

  runApp(const App());
}
