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
        script: 'cppcheck --enable=all --xml --xml-version=2 -i config -i ext -i support include src 2> build/cppcheck.xml',
        label: 'CPPCheck'
      )
      // recordIssues(
      //   id: 'tools-cppcheck',
      //   tool: cppCheck()
      // )      
    }
    // stage('tools/cloc/create') {
    //   createDirectory('build');
    //   sh 'cloc --by-file --exclude-dir=build,example,ext --xml --out=build/cloc.xml --quiet .';
    // }
    cleanWs()
  } // node('tools')
},
linux_gcc: {
  node('linux' && 'gcc') {
    stage('linux-gcc/scm') {
      deleteDir();
      checkoutGit();
    }
    stage('linux-gcc/build(make)') {
      cmakeBuild([
        buildDir: 'build-make',
        generator: 'Unix Makefiles',
        installation: "InSearchPath",
        steps: [[ args: "-- -j4", withCmake: true ]]
      ])
      recordIssues(
        id: 'linux-gcc',
        tool: gcc()
      )
    }
    stage('linux-gcc/build(ninja)') {
      cmakeBuild([
        buildDir: 'build-ninja',
        generator: 'Ninja',
        installation: "InSearchPath",
        steps: [[ args: "-- -j4", withCmake: true ]]
      ])
    }
    cleanWs()
  } // node('linux' && 'gcc')
},
linux_clang: {
  node('linux' && 'clang') {
    stage('linux-clang/scm') {
      deleteDir();
      checkoutGit();
    }
    stage('linux-clang/build(make)') {
      cmakeBuild([
        buildDir: 'build-make',
        generator: 'Unix Makefiles',
        installation: "InSearchPath",
        steps: [[ args: "-- -j4", withCmake: true ]]
      ])
      recordIssues(
        id: 'linux-clang',
        tool: clang()
      )
    }
    stage('linux-clang/build(ninja)') {
      cmakeBuild([
        buildDir: 'build-ninja',
        generator: 'Ninja',
        installation: "InSearchPath",
        steps: [[ args: "-- -j4", withCmake: true ]]
      ])
    }
    cleanWs()
  } // node('linux' && 'clang')
},
windows: {
    node('windows') {
    stage('windows/scm') {
      deleteDir();
      checkoutGit();
    }
    stage('windows/build(msvc)') {
      cmakeBuild([
        buildDir: 'build-msvc',
        generator: 'Visual Studio 16 2019',
        installation: "InSearchPath",
        steps: [[ args: "-- /nologo /verbosity:minimal /m:4", withCmake: true ]]
      ])
      recordIssues(
        id: 'windows-msbuild',
        tool: msBuild()
      )
    }
    stage('windows/build(ninja)') {
      bat(
        script: """
        call "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat" x64
        if not exist build-ninja mkdir build-ninja
        cd build-ninja
        cmake -G Ninja ..
        cmake --build . -- -j 4 all
        """,
        label: 'Generate build-scripts with cmake and execute them'
      ) 
    }
    cleanWs()
  } // node('windows')
},
macos: {
  node('macos') {
    stage('macos/scm') {
      deleteDir();
      checkoutGit();
    }
    stage('macos/build(make)') {
      cmakeBuild([
        buildDir: 'build-make',
        generator: 'Unix Makefiles',
        installation: "InSearchPath",
        steps: [[ args: "-- -j4", withCmake: true ]]
      ])
    }
    stage('macos/build(xcode)') {
      cmakeBuild([
        buildDir: 'build-xcode',
        generator: 'Xcode',
        installation: "InSearchPath",
        steps: [[ args: "-- -quiet -parallelizeTargets -jobs 4", withCmake: true ]]
      ])
    }
    cleanWs()
  } // node('macos')
}
