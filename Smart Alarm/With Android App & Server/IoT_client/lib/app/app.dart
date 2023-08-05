import 'package:flutter/material.dart';
import 'package:iot_client/about/about.dart';
import 'package:iot_client/chips/chips.dart';
import 'package:iot_client/home/home.dart';

class App extends StatelessWidget {
  const App({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      initialRoute: '/',
      title: 'IOT Project',
      routes: {
        '/': (context) => const HomePage(),
        '/chips': (context) => const ChipsPage(),
        '/about': (context) => const AboutPage(),
      },
    );
  }
}
