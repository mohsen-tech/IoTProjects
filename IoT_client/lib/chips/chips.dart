import 'package:flutter/material.dart';
import 'package:flutter_bloc/flutter_bloc.dart';
import 'package:iot_client/chips/bloc/chips_bloc.dart';
import 'package:iot_client/core/service_locator.dart';

class ChipsPage extends StatelessWidget {
  const ChipsPage({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return BlocProvider(
      create: (context) =>
          ServiceLocator.resolve<ChipsBloc>()..add(const ChipsEvent.loaded()),
      child: const ChipsView(),
    );
  }
}

class ChipsView extends StatelessWidget {
  const ChipsView({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('List'),
      ),
      body: BlocBuilder<ChipsBloc, ChipsState>(
        builder: (context, state) {
          return state.when(
            loadInProgress: () =>
                const Center(child: CircularProgressIndicator()),
            loadSuccess: (chips) => ListView.builder(
              padding: const EdgeInsets.symmetric(vertical: 8),
              itemCount: chips.length,
              itemBuilder: (context, index) {
                final chip = chips[index];
                return Container(
                  margin:
                      const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                  padding: const EdgeInsets.all(8),
                  decoration: BoxDecoration(
                    color: Colors.white,
                    borderRadius: BorderRadius.circular(8),
                    boxShadow: const [
                      BoxShadow(
                        offset: Offset(0, 1),
                        color: Colors.black26,
                        blurRadius: 2,
                        spreadRadius: 1,
                      ),
                    ],
                  ),
                  child: Row(
                    children: [
                      Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            'Chip Id',
                            style: Theme.of(context).textTheme.bodyText1,
                          ),
                          const SizedBox(height: 8),
                          Text(
                            'Date',
                            style: Theme.of(context).textTheme.bodyText1,
                          ),
                        ],
                      ),
                      const SizedBox(width: 8),
                      Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(chip.id.toString()),
                          const SizedBox(height: 8),
                          Text(
                            chip.createdAt.toString().substring(0, 10) +
                                ' ' +
                                chip.createdAt.toString().substring(11, 19),
                          ),
                        ],
                      ),
                    ],
                  ),
                );
              },
            ),
            loadFailure: () => Column(
              mainAxisAlignment: MainAxisAlignment.center,
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                const Icon(
                  Icons.wifi_off_outlined,
                  size: 64,
                ),
                const Padding(
                  padding: EdgeInsets.all(16),
                  child: Text(
                    'There was a problem connecting to the server.\nPlease check your network connection and try again.',
                    textAlign: TextAlign.center,
                  ),
                ),
                TextButton(
                  onPressed: () =>
                      context.read<ChipsBloc>().add(const ChipsEvent.loaded()),
                  child: const Text('Retry'),
                ),
              ],
            ),
          );
        },
      ),
    );
  }
}
