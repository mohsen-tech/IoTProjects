import 'package:flutter/material.dart';
import 'package:flutter_bloc/flutter_bloc.dart';
import 'package:iot_client/core/service_locator.dart';
import 'package:iot_client/home/bloc/chip_status_bloc.dart';

class HomePage extends StatelessWidget {
  const HomePage({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return BlocProvider(
      create: (context) => ServiceLocator.resolve<ChipStatusBloc>()
        ..add(const ChipStatusEvent.loaded()),
      child: const HomeView(),
    );
  }
}

class HomeView extends StatelessWidget {
  const HomeView({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Material(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Text(
            'IOT',
            style: Theme.of(context)
                .textTheme
                .headline1!
                .copyWith(letterSpacing: 10),
          ),
          Text(
            'Project',
            style: Theme.of(context)
                .textTheme
                .headline4!
                .copyWith(letterSpacing: 10, fontWeight: FontWeight.w300),
          ),
          const SizedBox(height: 64),
          SizedBox(
            width: MediaQuery.of(context).size.width / 3,
            height: 48,
            child: ElevatedButton(
              onPressed: () => Navigator.of(context).pushNamed('/chips'),
              child: const Text('List'),
            ),
          ),
          const SizedBox(height: 16),
          SizedBox(
            width: MediaQuery.of(context).size.width / 3,
            height: 48,
            child: ElevatedButton(
              onPressed: () => Navigator.of(context).pushNamed('/about'),
              child: const Text('About'),
            ),
          ),
        ],
      ),
    );
  }
}
