import 'package:flutter/material.dart';

class AboutPage extends StatelessWidget {
  const AboutPage({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('About'),
      ),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.start,
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: const [
            Text('ver: 0.0.0'),
            SizedBox(height: 32),
            Text('Mohsen AliMohammadi'),
            Text('96440296'),
            SizedBox(height: 32),
            Text('@mohsen_tech'),
          ],
        ),
      ),
    );
  }
}
