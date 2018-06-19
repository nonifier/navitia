pipeline {
  agent any
  stages {
    stage('Build') {
      agent {
        docker { image 'navitia/debian7_dev' }
      }
      steps {
        sh 'cmake -DCMAKE_BUILD_TYPE=Release ../source'
      }
    }
  }
}
