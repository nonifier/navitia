pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        echo 'Let\'s build navitia'
        dockerNode(dockerHost: 'ubuntu_18', image: 'https://raw.githubusercontent.com/CanalTP/navitia_docker_images/ubuntu_18/ubuntu18_dev/Dockerfile')
      }
    }
  }
}