import React, { useEffect, useState } from 'react';
import { ref, onValue, set } from 'firebase/database';
import database from '../firebase';
import '../App.css';

import {
  Chart as ChartJS,
  BarElement,
  CategoryScale,
  LinearScale,
  Tooltip,
  Legend,
} from 'chart.js';
import { Bar } from 'react-chartjs-2';

ChartJS.register(BarElement, CategoryScale, LinearScale, Tooltip, Legend);

const SUBJECTS = ['Math', 'Science', 'Social Studies', 'English'];
const CHOICES = ['A', 'B', 'C', 'D'];

const StudentDashboard = () => {
  const studentId = 'student1';

  const [rfid, setRfid] = useState('');
  const [studentName, setStudentName] = useState('');
  const [emotionValue, setEmotionValue] = useState(5);
  const [latestResponse, setLatestResponse] = useState(null);
  const [responseHistory, setResponseHistory] = useState([]);

  const [sharedPrompt, setSharedPrompt] = useState({
    question: '',
    subject: 'Math',
    correctAnswer: 'A',
  });

  const [teacherPromptInput, setTeacherPromptInput] = useState('');
  const [teacherSubject, setTeacherSubject] = useState('Math');
  const [teacherCorrectAnswer, setTeacherCorrectAnswer] = useState('A');

  useEffect(() => {
    const rfidRef = ref(database, `students/${studentId}/rfid`);
    const nameRef = ref(database, `students/${studentId}/name`);
    const emotionRef = ref(database, `students/${studentId}/emotion`);
    const promptRef = ref(database, 'currentPrompt');
    const responsesRef = ref(database, `students/${studentId}/responses`);

    onValue(rfidRef, (snap) => setRfid(snap.val() || ''));
    onValue(nameRef, (snap) => setStudentName(snap.val() || ''));
    onValue(emotionRef, (snap) => {
      const val = snap.val();
      setEmotionValue(val?.value || 5);
    });

    onValue(promptRef, (snap) => {
      const val = snap.val();
      setSharedPrompt(val || { question: '', subject: 'Math', correctAnswer: 'A' });
      setTeacherPromptInput(val?.question || '');
      setTeacherSubject(val?.subject || 'Math');
      setTeacherCorrectAnswer(val?.correctAnswer || 'A');
    });

    onValue(responsesRef, (snap) => {
      const val = snap.val();
      if (val) {
        const entries = Object.entries(val).map(([id, data]) => ({
          id,
          ...data,
        }));
        entries.sort((a, b) => b.timestamp - a.timestamp);
        setResponseHistory(entries);
        setLatestResponse(entries[0]);
      } else {
        setResponseHistory([]);
        setLatestResponse(null);
      }
    });
  }, []);

  const updatePrompt = () => {
    const promptRef = ref(database, 'currentPrompt');
    set(promptRef, {
      question: teacherPromptInput,
      subject: teacherSubject,
      correctAnswer: teacherCorrectAnswer,
    });
  };

  const getCorrectnessChartData = () => {
    const labels = SUBJECTS;
    const data = SUBJECTS.map((subject) => {
      const filtered = responseHistory.filter((r) => r.subject === subject);
      const total = filtered.length;
      const correct = filtered.filter((r) => r.isCorrect).length;
      return total > 0 ? (correct / total) * 100 : 0;
    });

    return {
      labels,
      datasets: [
        {
          label: 'Correctness Rate (%)',
          data,
          backgroundColor: 'rgba(75, 192, 192, 0.6)',
        },
      ],
    };
  };

  const getEmotionChartData = () => {
    const labels = SUBJECTS;
    const data = SUBJECTS.map((subject) => {
      const filtered = responseHistory.filter((r) => r.subject === subject);
      const total = filtered.length;
      return total > 0
        ? filtered.reduce((sum, r) => sum + (r.emotion || 0), 0) / total
        : 0;
    });

    return {
      labels,
      datasets: [
        {
          label: 'Average Emotion',
          data,
          backgroundColor: 'rgba(255, 159, 64, 0.6)',
        },
      ],
    };
  };

  return (
    <div className="dashboard">
      <h2>Desk Dashboard</h2>

      <div className="section">
        <p className="info"><strong>RFID:</strong> {rfid || 'Waiting for scan...'}</p>
        <p className="info"><strong>Student:</strong> {studentName || '-'}</p>
      </div>

      <div className="section">
        <label><strong>Emotion Level:</strong></label>
        <div style={{ backgroundColor: '#eee', height: '10px', borderRadius: '5px', width: '100%' }}>
          <div
            style={{
              width: `${(emotionValue / 10) * 100}%`,
              backgroundColor: '#36a2eb',
              height: '100%',
              borderRadius: '5px'
            }}
          />
        </div>
        <p className="info">Value: {emotionValue}</p>
      </div>

      <div className="section">
        <label><strong>Latest Answer:</strong></label>
        {latestResponse ? (
          <>
            <p className="info">
              Answer: <strong>{latestResponse.selectedAnswer}</strong>
            </p>
            <p className="info">
              Correct Answer: <strong>{latestResponse.correctAnswer}</strong>
            </p>
            <p className="info" style={{ color: latestResponse.isCorrect ? 'green' : 'red' }}>
              {latestResponse.isCorrect ? '✅ Correct' : '❌ Incorrect'}
            </p>
          </>
        ) : (
          <p className="info">No answer received yet.</p>
        )}
      </div>

      <div className="section">
        <p className="info"><strong>Current Prompt:</strong> {sharedPrompt.question}</p>
        <p className="info"><strong>Subject:</strong> {sharedPrompt.subject}</p>
        <p className="info"><strong>Correct Answer:</strong> {sharedPrompt.correctAnswer}</p>
      </div>

      <div className="section">
        <h3>Teacher: Set Prompt</h3>
        <label>Prompt:</label>
        <input
          type="text"
          value={teacherPromptInput}
          onChange={(e) => setTeacherPromptInput(e.target.value)}
        />

        <label>Subject:</label>
        <select value={teacherSubject} onChange={(e) => setTeacherSubject(e.target.value)}>
          {SUBJECTS.map((subj) => (
            <option key={subj} value={subj}>{subj}</option>
          ))}
        </select>

        <label>Correct Answer:</label>
        <select value={teacherCorrectAnswer} onChange={(e) => setTeacherCorrectAnswer(e.target.value)}>
          {CHOICES.map((ch) => (
            <option key={ch} value={ch}>{ch}</option>
          ))}
        </select>

        <button onClick={updatePrompt} style={{ marginTop: '10px' }}>
          Update Prompt
        </button>
      </div>

      <div className="section">
        <h3>Response History</h3>
        {responseHistory.length === 0 ? (
          <p>No responses yet.</p>
        ) : (
          <table style={{ width: '100%', borderCollapse: 'collapse' }}>
            <thead>
              <tr>
                <th>Time</th>
                <th>Question</th>
                <th>Subject</th>
                <th>Emotion</th>
                <th>Correct</th>
              </tr>
            </thead>
            <tbody>
              {responseHistory.map((r) => (
                <tr key={r.id}>
                  <td>{new Date(r.timestamp * 1000).toLocaleTimeString()}</td>
                  <td>{r.question}</td>
                  <td>{r.subject}</td>
                  <td>{r.emotion}</td>
                  <td style={{ color: r.isCorrect ? 'green' : 'red' }}>
                    {r.isCorrect ? '✅' : '❌'}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>

      <div className="section">
        <h3>Correctness by Subject</h3>
        <Bar data={getCorrectnessChartData()} options={{ responsive: true, scales: { y: { beginAtZero: true, max: 100 } } }} />
      </div>

      <div className="section">
        <h3>Average Emotion by Subject</h3>
        <Bar data={getEmotionChartData()} options={{ responsive: true, scales: { y: { beginAtZero: true, max: 10 } } }} />
      </div>
    </div>
  );
};

export default StudentDashboard;
