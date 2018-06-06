pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        sh 'mkdir build_src && cd build_src'
        sh 'cmake -DCMAKE_BUILD_TYPE=Release ../source'
      }
    }
  }
}