def checkoutGit() {
  def url = 'https://github.com/sgct/SGCT';
  def branch = env.BRANCH_NAME

  if (isUnix()) {
    sh "git clone --recursive --depth 1 ${url} --branch ${branch} --single-branch ."
  }
  else {
    bat "git clone --recursive --depth 1 ${url} --branch ${branch} --single-branch ."
  }
}

def createDirectory(dir) {
  cmake([installation: 'InSearchPath', arguments: "-E make_directory ${dir}"])
}

parallel tools: {
  node('tools') {
    stage('tools/scm') {
      deleteDir();
      checkoutGit();
    }
    stage('tools/cppcheck') {
      createDirectory('build');
      sh(
        script: 'cppcheck --enable=all --xml --xml-version=2 --suppressions-list=support/cppcheck/suppressions.txt -i config -i ext -i support include src 2> build/cppcheck.xml',
        label: 'CPPCheck'
      )
      recordIssues(
        id: 'tools-cppcheck',
        tool: cppCheck(pattern: 'build/cppcheck.xml')
      )      
    }
    // stage('tools/cloc/create') {
    //   createDirectory('build');
    //   sh 'cloc --by-file --exclude-dir=build,example,ext --xml --out=build/cloc.xml --quiet .';
    // }
    cleanWs()
  } // node('tools')
},
linux_gcc_make: {
  if (env.USE_BUILD_OS_LINUX == 'true') {
    node('linux' && 'gcc') {
      stage('linux-gcc-make/scm') {
        deleteDir();
        checkoutGit();
      }
      stage('linux-gcc-make/build') {
        cmakeBuild([
          buildDir: 'build-make',
          generator: 'Unix Makefiles',
          installation: "InSearchPath",
          cmakeArgs: "-DSGCT_EXAMPLES=ON",
          steps: [[ args: "-- -j4", withCmake: true ]]
        ])
        recordIssues(
          id: 'linux-gcc-make',
          tool: gcc()
        )
      }
      cleanWs()
    } // node('linux' && 'gcc')
  }
},
linux_gcc_ninja: {
  if (env.USE_BUILD_OS_LINUX == 'true') {
    node('linux' && 'gcc') {
      stage('linux-gcc-ninja/scm') {
        deleteDir();
        checkoutGit();
      }
      stage('linux-gcc-ninja/build') {
        cmakeBuild([
          buildDir: 'build-ninja',
          generator: 'Ninja',
          installation: "InSearchPath",
          cmakeArgs: "-DSGCT_EXAMPLES=ON",
          steps: [[ args: "-- -j4", withCmake: true ]]
        ])
      }
      cleanWs()
    } // node('linux' && 'gcc')
  }
},
linux_clang_make: {
  if (env.USE_BUILD_OS_LINUX == 'true') {
    node('linux' && 'clang') {
      stage('linux-clang-make/scm') {
        deleteDir();
        checkoutGit();
      }
      stage('linux-clang-make/build(make)') {
        cmakeBuild([
          buildDir: 'build-make',
          generator: 'Unix Makefiles',
          installation: "InSearchPath",
          cmakeArgs: "-DSGCT_EXAMPLES=ON",
          steps: [[ args: "-- -j4", withCmake: true ]]
        ])
        recordIssues(
          id: 'linux-clang-make',
          tool: clang()
        )
      }
      cleanWs()
    } // node('linux' && 'clang')
  }
},
linux_clang_ninja: {
  if (env.USE_BUILD_OS_LINUX == 'true') {
    node('linux' && 'clang') {
      stage('linux-clang-ninja/scm') {
        deleteDir();
        checkoutGit();
      }
      stage('linux-clang-ninja/build(ninja)') {
        cmakeBuild([
          buildDir: 'build-ninja',
          generator: 'Ninja',
          installation: "InSearchPath",
          cmakeArgs: "-DSGCT_EXAMPLES=ON",
          steps: [[ args: "-- -j4", withCmake: true ]]
        ])
      }
      cleanWs()
    } // node('linux' && 'clang')
  }
},
windows_msvc: {
  if (env.USE_BUILD_OS_WINDOWS == 'true') {
    node('windows') {
      stage('windows-msvc/scm') {
        deleteDir();
        checkoutGit();
      }
      stage('windows-msvc/build') {
        cmakeBuild([
          buildDir: 'build-msvc',
          generator: 'Visual Studio 16 2019',
          installation: "InSearchPath",
          cmakeArgs: "-DSGCT_EXAMPLES=ON",
          steps: [[ args: "-- /nologo /verbosity:minimal /m:4", withCmake: true ]]
        ])
        recordIssues(
          id: 'windows-msbuild-msvc',
          tool: msBuild()
        )
      }
      cleanWs()
    } // node('windows')
  }
},
windows_ninja: {
  if (env.USE_BUILD_OS_WINDOWS == 'true') {
    node('windows') {
      stage('windows-ninja/scm') {
        deleteDir();
        checkoutGit();
      }
      stage('windows-ninja/build') {
        bat(
          script: """
          call "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat" x64
          if not exist build-ninja mkdir build-ninja
          cd build-ninja
          cmake -G Ninja ..
          cmake --build . -DSGCT_EXAMPLES=ON -- -j 4 all
          """,
          label: 'Generate build-scripts with cmake and execute them'
        ) 
      }
      cleanWs()
    } // node('windows')
  }
},
macos_make: {
  if (env.USE_BUILD_OS_MACOS == 'true') {
    node('macos') {
      stage('macos-make/scm') {
        deleteDir();
        checkoutGit();
      }
      stage('macos-make/build') {
        cmakeBuild([
          buildDir: 'build-make',
          generator: 'Unix Makefiles',
          installation: "InSearchPath",
          cmakeArgs: "-DSGCT_EXAMPLES=ON",
          steps: [[ args: "-- -j4", withCmake: true ]]
        ])
      }
      cleanWs()
    } // node('macos')
  }
},
macos_ninja: {
  if (env.USE_BUILD_OS_MACOS == 'true') {
    node('macos') {
      stage('macos-xcode/scm') {
        deleteDir();
        checkoutGit();
      }
      stage('macos-xcode/build') {
        cmakeBuild([
          buildDir: 'build-xcode',
          generator: 'Xcode',
          installation: "InSearchPath",
          cmakeArgs: "-DSGCT_EXAMPLES=ON",
          steps: [[ args: "-- -quiet -parallelizeTargets -jobs 4", withCmake: true ]]
        ])
      }
      cleanWs()
    } // node('macos')
  }
}
