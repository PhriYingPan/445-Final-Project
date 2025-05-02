
import { initializeApp } from "firebase/app";
import { getAnalytics } from "firebase/analytics";

import { getDatabase } from 'firebase/database';

const firebaseConfig = {
  apiKey: "AIzaSyB5LXUTq_ogn-eP4Rr1b6CvjO5LBctx0sw",
  authDomain: "desklearningapp.firebaseapp.com",
  projectId: "desklearningapp",
  storageBucket: "desklearningapp.firebasestorage.app",
  messagingSenderId: "648644258749",
  appId: "1:648644258749:web:f2b4e088b7a155ac0780d4",
  measurementId: "G-SFXS701NDR"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const analytics = getAnalytics(app);
const database = getDatabase(app);
export default database;