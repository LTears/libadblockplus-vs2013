{
  'targets': [{
    'target_name': 'abpshell',
    'type': 'executable',
    'dependencies': [
      'libadblockplus.gyp:libadblockplus'
    ],
    'sources': [
      'src/Main.cpp',
      'src/Command.cpp',
      'src/GcCommand.cpp',
      'src/HelpCommand.cpp',
      'src/FiltersCommand.cpp',
      'src/MatchesCommand.cpp',
      'src/PrefsCommand.cpp',
      'src/SubscriptionsCommand.cpp'
    ],
    'xcode_settings': {
      'OTHER_LDFLAGS': ['-stdlib=libstdc++'],
    },
  }]
}
