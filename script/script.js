import http from 'k6/http';
import { sleep } from 'k6';

export const options = {
  vus: 1000, 
  duration: '30s', 
};

export default function () {
  const url = 'http://localhost:8080';

  http.get(url);

  sleep(1);
}
