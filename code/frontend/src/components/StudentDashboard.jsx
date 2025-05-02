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
const RAISE_HAND_CODE = 'E';
const ANSWER_CHOICES = ['A', 'B', 'C', 'D'];

const formatTimestamp = (raw) => {
  if (raw == null) return '-';
  const n = Number(raw);
  const ms = n > 1e12 ? n : n * 1000;
  return new Intl.DateTimeFormat('en-US', {
    timeZone: 'America/Chicago',
    year: 'numeric',
    month: 'short',
    day: '2-digit',
    hour: 'numeric',
    minute: '2-digit',
    second: '2-digit',
    hour12: true,
  }).format(new Date(ms));
};

const StudentDashboard = () => {
  const studentId = 'student1';

  const [rfid, setRfid] = useState('');
  const [studentName, setStudentName] = useState('');
  const [emotionValue, setEmotionValue] = useState(5);
  const [latestResponse, setLatestResponse] = useState(null);
  const [lastRaiseHand, setLastRaiseHand] = useState(null);
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
    const promptRef = ref(database, 'currentPrompt');
    const responsesRef = ref(database, `students/${studentId}/responses`);

    onValue(rfidRef, (snap) => setRfid(snap.val() || ''));
    onValue(nameRef, (snap) => setStudentName(snap.val() || ''));

    onValue(promptRef, (snap) => {
      const val = snap.val() || {};
      setSharedPrompt({
        question: val.question ?? '',
        subject: val.subject ?? 'Math',
        correctAnswer: val.correctAnswer ?? 'A',
      });
      setTeacherPromptInput(val.question ?? '');
      setTeacherSubject(val.subject ?? 'Math');
      setTeacherCorrectAnswer(val.correctAnswer ?? 'A');
    });

    onValue(responsesRef, (snap) => {
      const val = snap.val();
      if (!val) {
        setResponseHistory([]);
        setLatestResponse(null);
        setLastRaiseHand(null);
        setEmotionValue(5);
        return;
      }

      const entries = Object.entries(val).map(([id, data]) => ({ id, ...data }));
      entries.sort((a, b) => b.timestamp - a.timestamp);
      setResponseHistory(entries);

      setLatestResponse(entries[0]);
      setEmotionValue(entries[0]?.emotion ?? 5);

      const raiseEntry = entries.find((e) => e.selectedAnswer === RAISE_HAND_CODE);
      setLastRaiseHand(raiseEntry || null);
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
    const data = SUBJECTS.map((subject) => {
      const attempts = responseHistory.filter(
        (r) => r.subject === subject && r.selectedAnswer !== RAISE_HAND_CODE
      );
      const total = attempts.length;
      const correct = attempts.filter((r) => r.isCorrect).length;
      return total ? (correct / total) * 100 : 0;
    });
    return {
      labels: SUBJECTS,
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
    const data = SUBJECTS.map((subject) => {
      const filtered = responseHistory.filter((r) => r.subject === subject);
      const total = filtered.length;
      return total ? filtered.reduce((sum, r) => sum + (r.emotion || 0), 0) / total : 0;
    });
    return {
      labels: SUBJECTS,
      datasets: [
        {
          label: 'Average Emotion',
          data,
          backgroundColor: 'rgba(255, 159, 64, 0.6)',
        },
      ],
    };
  };

  const getWeakestSubject = () => {
    let lowestSubject = null;
    let lowestRate = Infinity;

    SUBJECTS.forEach((subject) => {
      const attempts = responseHistory.filter(
        (r) => r.subject === subject && r.selectedAnswer !== RAISE_HAND_CODE
      );
      const total = attempts.length;
      const correct = attempts.filter((r) => r.isCorrect).length;
      const rate = total ? (correct / total) * 100 : null;

      if (rate !== null && rate < lowestRate) {
        lowestRate = rate;
        lowestSubject = subject;
      }
    });

    return lowestSubject ? `${lowestSubject} (${lowestRate.toFixed(1)}%)` : 'N/A';
  };

  const renderAnswerCell = (answer) =>
    answer === RAISE_HAND_CODE ? '🖐️ Raise Hand' : answer;

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
              borderRadius: '5px',
            }}
          />
        </div>
        <p className="info">Value: {emotionValue}</p>
      </div>

      <div className="section">
        <label><strong>Latest Event:</strong></label>
        {latestResponse ? (
          latestResponse.selectedAnswer === RAISE_HAND_CODE ? (
            <p className="info" style={{ color: '#db8b00', fontWeight: 'bold' }}>
              🖐️ Student Raised Hand ({formatTimestamp(latestResponse.timestamp)})
            </p>
          ) : (
            <>
              <p className="info">Answer: <strong>{latestResponse.selectedAnswer}</strong></p>
              <p className="info">Correct Answer: <strong>{latestResponse.correctAnswer}</strong></p>
              <p className="info">Emotion: <strong>{latestResponse.emotion}</strong></p>
              <p className="info" style={{ color: latestResponse.isCorrect ? 'green' : 'red' }}>
                {latestResponse.isCorrect ? '✅ Correct' : '❌ Incorrect'}
              </p>
            </>
          )
        ) : (
          <p className="info">No events yet.</p>
        )}

        {lastRaiseHand && latestResponse?.id !== lastRaiseHand.id && (
          <p className="info" style={{ color: '#db8b00', fontWeight: 'bold' }}>
            🖐️ Student Raised Hand ({formatTimestamp(lastRaiseHand.timestamp)})
          </p>
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
        <select
          value={teacherCorrectAnswer}
          onChange={(e) => setTeacherCorrectAnswer(e.target.value)}
        >
          {ANSWER_CHOICES.map((ch) => (
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
                <th>Event</th>
                <th>Correct</th>
              </tr>
            </thead>
            <tbody>
              {responseHistory.map((r) => (
                <tr key={r.id}>
                  <td>{formatTimestamp(r.timestamp)}</td>
                  <td>{r.question}</td>
                  <td>{r.subject}</td>
                  <td>{r.emotion}</td>
                  <td>{renderAnswerCell(r.selectedAnswer)}</td>
                  <td style={{ color: r.isCorrect ? 'green' : 'red' }}>
                    {r.selectedAnswer === RAISE_HAND_CODE ? '-' : r.isCorrect ? '✅' : '❌'}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>

      <div className="section">
        <h3>Struggling Subject</h3>
        <p className="info">
          <strong>Subject with lowest correctness:</strong> {getWeakestSubject()}
        </p>
      </div>

      <div className="section">
        <h3>Correctness by Subject</h3>
        <Bar
          data={getCorrectnessChartData()}
          options={{ responsive: true, scales: { y: { beginAtZero: true, max: 100 } } }}
        />
      </div>

      <div className="section">
        <h3>Average Emotion by Subject</h3>
        <Bar
          data={getEmotionChartData()}
          options={{ responsive: true, scales: { y: { beginAtZero: true, max: 10 } } }}
        />
      </div>
    </div>
  );
};

export default StudentDashboard;
